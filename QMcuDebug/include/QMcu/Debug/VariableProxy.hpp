#pragma once

#include <QJSValue>
#include <QObject>
#include <QtQmlIntegration>

#include <QMcu/Debug/Variable.hpp>
#include <QMcu/Debug/VariableProxyGroup.hpp>

class VariableProxy : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(Variable* variable READ variable NOTIFY variableChanged)
  Q_PROPERTY(QVariant value READ value NOTIFY valueChanged)
  Q_PROPERTY(QJSValue transform READ transform WRITE setTransform NOTIFY transformChanged)
  Q_PROPERTY(VariableProxyGroup* group READ group WRITE setGroup NOTIFY groupChanged)

public:
  explicit VariableProxy(QObject* parent = nullptr);
  virtual ~VariableProxy() = default;

  QString const& name() const noexcept
  {
    return name_;
  }

  Variable* variable() noexcept
  {
    return variable_;
  }

  QVariant const& value() const noexcept
  {
    return value_;
  }

  Q_INVOKABLE int readInt8() const noexcept
  {
    return variable_->read<int8_t>().value_or(0);
  }

  Q_INVOKABLE int readUInt8() const noexcept
  {
    return variable_->read<uint8_t>().value_or(0);
  }

  Q_INVOKABLE int readInt16() const noexcept
  {
    return variable_->read<int16_t>().value_or(0);
  }

  Q_INVOKABLE int readUInt16() const noexcept
  {
    return variable_->read<uint16_t>().value_or(0);
  }

  Q_INVOKABLE int readInt32() const noexcept
  {
    return variable_->read<int32_t>().value_or(0);
  }

  Q_INVOKABLE int readUInt32() const noexcept
  {
    return variable_->read<uint32_t>().value_or(0);
  }

  Q_INVOKABLE int readInt64() const noexcept
  {
    return variable_->read<int64_t>().value_or(0);
  }

  Q_INVOKABLE int readUInt64() const noexcept
  {
    return variable_->read<uint64_t>().value_or(0);
  }

  Q_INVOKABLE int readReal32() const noexcept
  {
    return variable_->read<float>().value_or(0);
  }

  Q_INVOKABLE int readReal64() const noexcept
  {
    return variable_->read<double>().value_or(0);
  }

  QJSValue const& transform() const noexcept
  {
    return transform_;
  }

  VariableProxyGroup* group() noexcept;

public slots:
  void update();
  void setName(QString const& name);
  void setTransform(QJSValue const& fn);
  void setGroup(VariableProxyGroup* group);

signals:
  void nameChanged();
  void variableChanged();
  void variableResolved();
  void valueChanged();
  void valueUnChanged();
  void transformChanged();
  void triggered();
  void groupChanged();

private slots:
  void refresh();

private:
  static inline Debugger* debugger() noexcept;

  QString             name_;
  QJSValue            transform_;
  Variable*           variable_ = nullptr;
  VariableProxyGroup* group_    = nullptr;
  QVariant            value_;
};
