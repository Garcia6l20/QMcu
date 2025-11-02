#pragma once

#include <QMcu/Debug/VariableProxy.hpp>
#include <QMcu/Plot/AbstractPlotDataProvider.hpp>

#include <QElapsedTimer>
#include <QQmlComponent>

#include <qqmlintegration.h>

#include <lldb/API/LLDB.h>

class Debugger;

class AbstractVariablePlotDataProvider : public AbstractPlotDataProvider
{
  Q_OBJECT
  Q_CLASSINFO("DefaultProperty", "proxy")
  Q_PROPERTY(VariableProxy* proxy READ proxy WRITE setProxy NOTIFY proxyChanged)

public:
  AbstractVariablePlotDataProvider(QObject* parent = nullptr);
  virtual ~AbstractVariablePlotDataProvider() = default;

  VariableProxy* proxy() noexcept
  {
    return proxy_;
  }

  static qint64 msTime() noexcept;
  static qint64 nsTime() noexcept;
  static qreal  sTime() noexcept;

public slots:
  void setProxy(VariableProxy* proxy);

signals:
  void proxyChanged();

protected:
  Variable*        variable();
  static Debugger* debugger();

  virtual void onValueChanged() = 0;
  virtual void onValueUnChanged() {};

private:
  static QElapsedTimer    time_;
  VariableProxy*          proxy_ = nullptr;
  QMetaObject::Connection proxyValueChangedConnection_;
  QMetaObject::Connection proxyValueUnChangedConnection_;
};
