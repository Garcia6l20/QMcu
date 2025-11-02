#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include <lldb/API/LLDB.h>

class Variable;

class Type : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("created by Debugger")

  Q_PROPERTY(QString name READ name CONSTANT)

public:
  Type(lldb::SBType type, Variable* parent);
  virtual ~Type() = default;

  QString name();

  inline bool isBasic()
  {
    return canonicalBasicType() != lldb::eBasicTypeInvalid;
  }

  inline lldb::BasicType canonicalBasicType()
  {
    return type_.GetCanonicalType().GetBasicType();
  }

  inline bool isArray()
  {
    return type_.IsArrayType();
  }

  inline size_t size() noexcept
  {
    return type_.GetByteSize();
  }

  inline size_t align() noexcept
  {
    return type_.GetByteAlign();
  }

private:
  inline Variable* variable();
  lldb::SBType     type_;
};