#include <QMcu/Debug/AbstractVariablePlotDataProvider.hpp>
#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/StLinkProbe.hpp>
#include <QMcu/Debug/Type.hpp>
#include <QMcu/Debug/VariableProxy.hpp>

#include <QDebug>
#include <QTimerEvent>

#include <Logging.hpp>

#include <QTimer>

#include <magic_enum/magic_enum.hpp>

using namespace lldb;

QElapsedTimer AbstractVariablePlotDataProvider::time_ = []
{
  QElapsedTimer timer;
  timer.start();
  return timer;
}();

qint64 AbstractVariablePlotDataProvider::msTime() noexcept
{
  return time_.elapsed();
}

qint64 AbstractVariablePlotDataProvider::nsTime() noexcept
{
  return time_.nsecsElapsed();
}

qreal AbstractVariablePlotDataProvider::sTime() noexcept
{
  return time_.elapsed() / 1000.;
}

AbstractVariablePlotDataProvider::AbstractVariablePlotDataProvider(QObject* parent)
    : AbstractPlotDataProvider(parent)
{
}

Debugger* AbstractVariablePlotDataProvider::debugger()
{
  return Debugger::instance();
}

Variable* AbstractVariablePlotDataProvider::variable()
{
  if(proxy_)
  {
    return proxy_->variable();
  }
  return nullptr;
}

void AbstractVariablePlotDataProvider::setProxy(VariableProxy* proxy)
{
  if(proxy != proxy_)
  {
    disconnect(proxyValueChangedConnection_);
    disconnect(proxyValueUnChangedConnection_);
    proxy_ = proxy;
    if(name().isEmpty())
    {
      setName(proxy_->name());
    }
    proxyValueChangedConnection_ = connect(proxy_,
                                           &VariableProxy::valueChanged,
                                           this,
                                           &AbstractVariablePlotDataProvider::onValueChanged);
    proxyValueChangedConnection_ = connect(proxy_,
                                           &VariableProxy::valueUnChanged,
                                           this,
                                           &AbstractVariablePlotDataProvider::onValueUnChanged);
    emit proxyChanged();
  }
}
