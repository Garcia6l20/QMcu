#pragma once

#include <QObject>
#include <QVariant>

#include <qqmlintegration.h>

#include <lldb/API/LLDB.h>

#include <QMcu/Debug/Type.hpp>

class Debugger;

class Variable : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("created by Debugger")

  Q_PROPERTY(QString display READ display CONSTANT)
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(Type* type READ type CONSTANT)
  Q_PROPERTY(uint64_t address READ address CONSTANT)

  Q_PROPERTY(uint64_t arrayElementCount READ arrayElementCount WRITE setArrayElementCount NOTIFY
                 arrayElementCountChanged)
  Q_PROPERTY(uint64_t arrayElementOffset READ arrayElementOffset WRITE setArrayElementOffset NOTIFY
                 arrayElementOffsetChanged)

public:
  Variable(lldb::SBValue value, Debugger* parent);
  virtual ~Variable() = default;

  QString        name();
  QString        display();
  Type*          type();
  lldb::SBValue& value()
  {
    return value_;
  }
  uint64_t address();

  bool read(std::span<std::byte> data);

  template <typename T> std::optional<T> read()
  {
    T value;
    if(not read(std::as_writable_bytes(std::span{&value, 1})))
    {
      return std::nullopt;
    }
    return value;
  }

  qreal readAsReal();

  inline QVariant const& read() noexcept
  {
    loadValue_(local_);
    return local_;
  }

  uint64_t arrayElementCount() const noexcept
  {
    return arrayElementCount_;
  }

  uint64_t arrayElementOffset() const noexcept
  {
    return arrayElementOffset_;
  }

public slots:

  void setArrayElementCount(uint64_t arrayElementCount)
  {
    if(arrayElementCount != arrayElementCount_)
    {
      arrayElementCount_ = arrayElementCount;
      resolve(); // re-resolve
      emit arrayElementCountChanged(arrayElementCount_);
    }
  }

  void setArrayElementOffset(uint64_t arrayElementOffset)
  {
    if(arrayElementOffset != arrayElementCount_)
    {
      arrayElementOffset_ = arrayElementOffset;
      resolve(); // re-resolve
      emit arrayElementOffsetChanged(arrayElementOffset_);
    }
  }

signals:
  void arrayElementCountChanged(uint64_t);
  void arrayElementOffsetChanged(uint64_t);

private:
  inline Debugger*               debugger();
  void                           resolve();
  lldb::SBValue                  value_;
  QVariant                       local_;
  QVariant                       cache_;
  std::function<bool(QVariant&)> loadValue_;
  uint64_t                       arrayElementCount_  = std::numeric_limits<uint64_t>::max();
  uint64_t                       arrayElementOffset_ = 0;
};
