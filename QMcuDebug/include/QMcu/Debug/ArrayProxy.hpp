#pragma once

#include <QJSValue>
#include <QObject>
#include <QtQmlIntegration>

#include <QMcu/Debug/VariableProxy.hpp>
#include <QMcu/Debug/VariableProxyGroup.hpp>

class ArrayProxy : public VariableProxy
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(size_t size READ size NOTIFY sizeChanged)

public:
  explicit ArrayProxy(QObject* parent = nullptr);
  virtual ~ArrayProxy() = default;

public slots:
  size_t size() noexcept
  {
    if(size_ != std::numeric_limits<size_t>::max())
    {
      return size_;
    }
    return 0;
  }

signals:
  void sizeChanged();

private slots:

private:
  size_t size_ = std::numeric_limits<size_t>::max();
};
