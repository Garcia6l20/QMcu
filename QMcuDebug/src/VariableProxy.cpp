#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/StLinkProbe.hpp>
#include <QMcu/Debug/VariableProxy.hpp>
#include <QMcu/Debug/VariableProxyGroup.hpp>

#include <Logging.hpp>

#include <QQmlEngine>

Debugger* VariableProxy::debugger() noexcept
{
  return Debugger::instance();
}

VariableProxy::VariableProxy(QObject* parent) : QObject{parent}
{
  connect(Debugger::instance(),
          &Debugger::readyChanged,
          this,
          [this](bool ready)
          {
            if(ready)
            {
              auto dbg  = debugger();
              variable_ = dbg->variable(name_);
              if(variable_ == nullptr)
              {
                qCritical() << "Cannot find variable:" << name_;
                return;
              }
              emit variableChanged();
              if(StLinkProbe::instance() == nullptr)
              {
                connect(dbg, &Debugger::processStopped, this, &VariableProxy::refresh);
              }
            }
          });
}

VariableProxyGroup* VariableProxy::group() noexcept
{
  return group_;
}

void VariableProxy::setGroup(VariableProxyGroup* group)
{
  if(group != group_)
  {
    if(group_ != nullptr)
    {
      group_->removeProxy(this);
    }
    group_ = group;
    if(group_ != nullptr)
    {
      group_->addProxy(this);
    }
    emit groupChanged();
  }
}

void VariableProxy::setName(QString const& name)
{
  if(name != name_)
  {
    name_ = name;
    if(debugger()->ready())
    {
      variable_ = debugger()->variable(name);
      if(variable_ == nullptr)
      {
        throw std::runtime_error(std::format("cannot find variable {}", name_.toUtf8()));
      }
      emit variableChanged();
    }
    emit nameChanged();
  }
}

void VariableProxy::setTransform(QJSValue const& fn)
{
  if(not fn.isCallable())
  {
    qCritical(lcWatcher) << fn.toString() << "is not callable";
  }
  if(fn.strictlyEquals(transform_))
  {
    return;
  }
  transform_ = fn;
  emit transformChanged();
}

void VariableProxy::update()
{
  if(StLinkProbe::instance())
  {
    refresh();
  }
  else
  {
    Debugger::instance()->process().Stop();
  }
}

void VariableProxy::refresh()
{
  if(variable_ == nullptr)
  {
    return;
  }

  auto value = variable_->read();
  if(value != value_)
  {
    if(transform_.isCallable())
    {
      QQmlEngine* const e = qmlEngine(this);
      value_              = transform_.call(QJSValueList() << e->toScriptValue(value)).toVariant();
    }
    else
    {
      value_ = std::move(value);
    }
    emit valueChanged();
  }
  else
  {
    emit valueUnChanged();
  }
  emit triggered();
}
