#include <QMcu/Debug/AbstractVariableRecorder.hpp>
#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/StLinkProbe.hpp>
#include <QMcu/Debug/Type.hpp>
#include <QMcu/Debug/VariableProxy.hpp>

#include <QDebug>
#include <QTimerEvent>

#include <Logging.hpp>

#include <QTimer>

#include <magic_enum/magic_enum.hpp>

Q_LOGGING_CATEGORY(lcWatcher, "qmcu.watcher")

using namespace lldb;

QElapsedTimer AbstractVariableRecorder::time_ = []
{
  QElapsedTimer timer;
  timer.start();
  return timer;
}();

qint64 AbstractVariableRecorder::msTime() noexcept
{
  return time_.elapsed();
}

qint64 AbstractVariableRecorder::nsTime() noexcept
{
  return time_.nsecsElapsed();
}

qreal AbstractVariableRecorder::sTime() noexcept
{
  return time_.elapsed() / 1000.;
}

AbstractVariableRecorder::AbstractVariableRecorder(QObject* parent) : QObject(parent)
{
}

Debugger* AbstractVariableRecorder::debugger()
{
  return Debugger::instance();
}

Variable* AbstractVariableRecorder::variable()
{
  if(proxy_)
  {
    return proxy_->variable();
  }
  return nullptr;
}

void AbstractVariableRecorder::setProxy(VariableProxy* proxy)
{
  if(proxy != proxy_)
  {
    disconnect(proxyValueChangedConnection_);
    disconnect(proxyValueUnChangedConnection_);
    proxy_                       = proxy;
    proxyValueChangedConnection_ = connect(proxy_,
                                           &VariableProxy::valueChanged,
                                           this,
                                           &AbstractVariableRecorder::onValueChanged);
    proxyValueChangedConnection_ = connect(proxy_,
                                           &VariableProxy::valueUnChanged,
                                           this,
                                           &AbstractVariableRecorder::onValueUnChanged);
    emit proxyChanged();
  }
}
