#pragma once

#include <QMcu/Debug/Variable.hpp>

#include <QObject>
#include <QUrl>

#include <qqmlintegration.h>

#include <lldb/API/LLDB.h>


#include <thread>

class Debugger : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QList<Variable*> globals READ globals NOTIFY targetLoadingCompleted)
  Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
  Q_PROPERTY(bool launching READ launching NOTIFY launchedChanged)
  Q_PROPERTY(bool launched READ launched WRITE launchProcess NOTIFY launchedChanged)
  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(QString executable READ executable WRITE load NOTIFY launchedChanged)
  Q_PROPERTY(QString targetArchitecture READ targetArchitecture NOTIFY launchedChanged)

public:
  Debugger(QObject* parent = nullptr);
  virtual ~Debugger();

  static Debugger* instance()
  {
    return instance_;
  }

  QList<Variable*> globals();
  bool             ready();
  bool             launched()
  {
    return process_.IsValid();
  }
  bool launching()
  {
    return launching_;
  }
  bool running()
  {
    return process_.IsValid() and process_.GetState() == lldb::eStateRunning;
  }
  lldb::SBTarget target()
  {
    return target_;
  }
  lldb::SBProcess process()
  {
    return process_;
  }
  QString executable()
  {
    if(module_.IsValid())
    {
      return module_.GetFileSpec().GetFilename();
    }
    return {};
  }
  QString targetArchitecture();

  Q_INVOKABLE Variable* variable(QString const& name);

public slots:
  void load(QUrl const& executable);
  void load(QString const& executable);
  bool launchProcess(bool);

signals:
  void targetLoadingStarted();
  void targetLoadingCompleted();
  void readyChanged(bool);
  void launchingChanged(bool);
  void launchedChanged(bool);
  void runningChanged(bool);

  void processStopped();

  void continueProcess();

private:
  QList<Variable*> globals_;
  lldb::SBDebugger debugger_;
  lldb::SBTarget   target_;
  lldb::SBModule   module_;
  lldb::SBProcess  process_;
  bool             launching_ = false;
  lldb::SBListener listener_{"QMcu.Debugger"};
  std::jthread     listener_thread_;
  static Debugger* instance_;
};
