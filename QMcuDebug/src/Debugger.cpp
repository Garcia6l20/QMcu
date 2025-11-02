#include <QMcu/Debug/Debugger.hpp>

#include <QDebug>
#include <QTimer>

#include <thread>

#include <magic_enum/magic_enum.hpp>

using namespace lldb;

Debugger* Debugger::instance_ = nullptr;

#include <Logging.hpp>

Q_LOGGING_CATEGORY(lcDebugger, "qmcu.debugger")
Q_LOGGING_CATEGORY(lcDebuggerLLDB, "qmcu.debugger.lldb")

Debugger::Debugger(QObject* parent) : QObject(parent)
{
  if(instance_ != nullptr)
  {
    throw std::runtime_error("debugger is a singleton");
  }
  instance_ = this;
  SBDebugger::Initialize();
  debugger_ = SBDebugger::Create();
  debugger_.SetAsync(true);
  // debugger_.SetAsync(false);

  // see logging categories at https://github.com/vadimcn/codelldb/wiki/LLDB-Logging
  static const char* categories[] = {
      // "all", // all available logging categories
      // "default", // default set of logging categories
      // "api", // log API calls and return values
      // "ast", // log AST
      // "break", // log breakpoints
      // "commands", // log command argument parsing
      // "comm", // log communication activities
      // "conn", // log connection details
      // "demangle", // log mangled names to catch demangler crashes
      // "dyld", // log shared library related activities
      // "event", // log broadcaster, listener and event queue activities
      // "expr", // log expressions
      // "formatters", // log data formatters related activities
      // "host", // log host activities
      // "jit", // log JIT events in the target
      // "language", // log language runtime events
      // "mmap", // log mmap related activities
      // "module", // log module activities such as when modules are created, destroyed, replaced,
      // and more
      // "object", // log object construction/destruction for important objects
      // "os", // log OperatingSystem plugin related activities
      // "platform", // log platform events and activities
      // "process", // log process events and activities
      // "script", // log events about the script interpreter
      // "state", // log private and public process state changes
      // "step", // log step related activities
      // "symbol", // log symbol related issues and warnings
      // "system", //runtime - log system runtime events
      // "target", // log target events and activities
      // "temp", // log internal temporary debug messages
      // "thread", // log thread events and activities
      // "types", // log type system related activities
      // "unwind", // log stack unwind activities
      // "watch", // log watchpoint related activities
      // "on-demand", // log symbol on-demand related activities
      nullptr};
  if constexpr((sizeof(categories) / sizeof(categories[0]) > 1))
  {
    bool ok = debugger_.EnableLog("lldb", categories);
    assert(ok);
  }

  // Set a callback to receive log messages
  debugger_.SetLoggingCallback(
      [](const char* msg, void* userdata)
      {
        auto* self = static_cast<Debugger*>(userdata);
        qDebug(lcDebuggerLLDB) << msg;
      },
      this);

  connect(this, &Debugger::targetLoadingStarted, this, [this] { emit readyChanged(false); });
  connect(this, &Debugger::targetLoadingCompleted, this, [this] { emit readyChanged(ready()); });

  connect(this, &Debugger::continueProcess, this, [this] { process_.Continue(); });

  listener_thread_ = std::jthread(
      [this](std::stop_token stop)
      {
        lldb::SBEvent event;
        while(not stop.stop_requested())
        {
          if(listener_.WaitForEvent(1, event))
          { // 1 second timeout
            if(lldb::SBProcess::EventIsProcessEvent(event))
            {
              auto state = lldb::SBProcess::GetStateFromEvent(event);
              // qDebug(lcDebugger) << "State" << magic_enum::enum_name(state);
              if(launching_)
              {
                launching_ = false;
                emit launchingChanged(false);
              }
              emit runningChanged(state == lldb::eStateRunning);
              if(state == lldb::eStateStopped)
              {
                emit processStopped();
                emit continueProcess();
              }
            }
          }
        }
      });
}

Debugger::~Debugger()
{
  listener_thread_.request_stop();
  SBDebugger::Terminate();
  instance_ = nullptr;
}

bool Debugger::ready()
{
  return target_.IsValid() and module_.IsValid();
}

void Debugger::load(QString const& executable)
{
  for(auto* var : globals_)
  {
    var->deleteLater();
  }
  globals_.clear();
  target_ = SBTarget{};
  module_ = SBModule{};

  emit targetLoadingStarted();

  std::jthread(
      [this, executable]
      {
        qDebug(lcDebugger) << "Loading target" << executable << "...";
        target_ = debugger_.CreateTarget(executable.toUtf8());
        if(!target_.IsValid())
        {
          target_ = SBTarget{};
          qCritical(lcDebugger) << "Failed to create target";
          emit targetLoadingCompleted();
          return;
        }
        qDebug(lcDebugger) << "Loading module...";
        module_ = target_.GetModuleAtIndex(0);
        if(!module_.IsValid())
        {
          target_ = SBTarget{};
          qCritical(lcDebugger) << "Failed to load module";
          emit targetLoadingCompleted();
          return;
        }
        qDebug(lcDebugger) << "Target" << executable << "loaded !";
        emit targetLoadingCompleted();
        // QTimer::singleShot(1000, [this] { emit targetLoadingCompleted(); });
      })
      .detach();
  // qDebug() << "load exited !";
}

void Debugger::load(QUrl const& executable)
{
  load(executable.toLocalFile());
}

QList<Variable*> Debugger::globals()
{
  if(globals_.empty())
  {
    const size_t n = module_.GetNumSymbols();
    if(n > 0)
    {
      qDebug(lcDebugger) << "Loading global variables...";
      QList<Variable*> out(n);
      for(size_t ii = 0; ii < n; ++ii)
      {
        auto sym = module_.GetSymbolAtIndex(ii);
        if(sym.IsValid() and sym.GetName() and sym.GetType() == lldb::eSymbolTypeData)
        {
          const auto s_name = sym.GetName();
          auto       value  = target_.FindGlobalVariables(s_name, 1).GetValueAtIndex(0);
          if(not value.IsValid())
          {
            qDebug(lcDebugger) << "Skipping symbol" << s_name << "(invalid)";
            continue;
          }
          if(not value.GetName())
          {
            qDebug(lcDebugger) << "Skipping symbol" << s_name << "(unamed value)";
            continue;
          }
          globals_.append(new Variable(value, this));
        }
      }
      qDebug(lcDebugger) << "Global variables loaded !";
    }
  }
  return globals_;
}

// #include <lldb/Host/HostInfoBase.h>

Variable* Debugger::variable(QString const& name)
{
  const auto parts = name.split('.', Qt::SkipEmptyParts);
  if(parts.isEmpty())
    return nullptr;

  auto value = target_.FindGlobalVariables(parts[0].toUtf8(), 1).GetValueAtIndex(0);
  if(not value.IsValid())
  {
    qCritical(lcDebugger) << "Skipping symbol" << name << "(invalid)";
    return nullptr;
  }
  for(int i = 1; i < parts.size(); ++i)
  {
    const QByteArray child_name = parts[i].toUtf8();
    SBValue          child      = value.GetChildMemberWithName(child_name.constData());
    if(!child.IsValid())
    {
      qCritical(lcDebugger) << "Failed to resolve member" << parts[i] << "in" << parts[i - 1];
      return nullptr;
    }
    value = child;
  }

  if(!value.IsValid())
  {
    qCritical(lcDebugger) << "Skipping symbol" << name << "(invalid)";
    return nullptr;
  }
  if(!value.GetName())
  {
    qCritical(lcDebugger) << "Skipping symbol" << name << "(no name)";
    return nullptr;
  }
  return new Variable(value, this);
}

QString Debugger::targetArchitecture()
{
  if(not target_.IsValid())
  {
    return "<target-not-loaded>";
  }
  // auto triple = std::string_view{target_.GetTriple()};
  // auto spec = lldb_private::HostInfoBase::GetAugmentedArchSpec(target_.GetTriple());
  // return spec.GetArchitectureName();
  auto triple = QString{target_.GetTriple()};
  return triple.split('-')[0];
}

bool Debugger::launchProcess(bool launch)
{
  const bool was_launched = this->launched();
  if(launch != was_launched)
  {
    if(launch)
    {
      if(launching_)
      {
        return true;
      }
      launching_ = true;
      emit launchingChanged(true);

      std::jthread(
          [this]
          {
            qDebug(lcDebugger) << "Launching process...";

            SBLaunchInfo launch_info(nullptr);
            launch_info.SetLaunchFlags(eLaunchFlagDisableASLR);
            launch_info.SetWorkingDirectory(".");
            launch_info.SetListener(listener_);

            SBError error;
            process_ = target_.Launch(launch_info, error);

            if(error.Fail() or not process_.IsValid())
            {
              qCritical(lcDebugger) << "Launch failed: " << error.GetCString();
            }
            else
            {
              qInfo(lcDebugger) << "Process started, pid=" << process_.GetProcessID();

              if(process_.GetState() != lldb::eStateRunning)
              {
                qCritical() << "Process not running" << magic_enum::enum_name(process_.GetState());
                if(process_.GetState() == lldb::eStateExited)
                {
                  qCritical(lcDebugger) << "Process exitted: " << process_.GetExitDescription();
                }
                else if(process_.GetState() == lldb::eStateStopped)
                {
                  process_.Continue();
                }
              }
              else
              {
                launching_ = false;
                emit launchingChanged(false);
              }

              emit launchedChanged(this->launched());
            }
          })
          .detach();
    }
    else
    {
      process_.Kill();
      process_ = SBProcess();
    }
  }
  // const bool success = launch == this->launched();
  // if(launch != was_launched)
  // {
  //   emit launchedChanged(this->launched());
  // }
  // return success;
  return true;
}