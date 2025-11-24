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

  inline size_t sizeBytes() noexcept
  {
    return type_.GetByteSize();
  }

  inline size_t align() noexcept
  {
    return type_.GetByteAlign();
  }

  static QMetaType::Type getQtType(lldb::SBType& type)
  {
    auto       ctype = type.GetCanonicalType();
    const auto btype = ctype.GetBasicType();
    assert(btype != lldb::eBasicTypeInvalid);
    const auto size = type.GetByteSize();
    switch(btype)
    {
      case lldb::eBasicTypeBool:
        assert(sizeof(bool) == size);
        return QMetaType::Bool;
      case lldb::eBasicTypeChar:
      case lldb::eBasicTypeChar8:
      case lldb::eBasicTypeSignedChar:
        assert(sizeof(int8_t) == size);
        return QMetaType::Char;
      case lldb::eBasicTypeChar16:
      case lldb::eBasicTypeSignedWChar:
      case lldb::eBasicTypeShort:
      case lldb::eBasicTypeHalf:
        assert(sizeof(int16_t) == size);
        return QMetaType::Short;
      case lldb::eBasicTypeInt:
      case lldb::eBasicTypeLong:
        assert(sizeof(int32_t) == size);
        return QMetaType::Int;
      case lldb::eBasicTypeLongLong:
        assert(sizeof(int64_t) == size);
        return QMetaType::LongLong;
      case lldb::eBasicTypeUnsignedChar:
        assert(sizeof(uint8_t) == size);
        return QMetaType::UChar;
      case lldb::eBasicTypeUnsignedShort:
        assert(sizeof(uint16_t) == size);
        return QMetaType::UShort;
      case lldb::eBasicTypeUnsignedInt:
      case lldb::eBasicTypeUnsignedLong:
        assert(sizeof(uint32_t) == size);
        return QMetaType::UInt;
      case lldb::eBasicTypeUnsignedLongLong:
        assert(sizeof(uint64_t) == size);
        return QMetaType::ULongLong;
      case lldb::eBasicTypeFloat:
        assert(sizeof(float) == size);
        return QMetaType::Float;
      case lldb::eBasicTypeDouble:
        assert(sizeof(double) == size);
        return QMetaType::Double;
      default:
        return QMetaType::Void;
    }
  }

  inline lldb::SBType& elementType() noexcept
  {
    return elementType_;
  }

  inline QMetaType::Type elementTypeId()
  {
    return getQtType(elementType_);
  }

  inline size_t elementSize() noexcept
  {
    return elementType_.GetByteSize();
  }

  inline QList<int> const& extents() noexcept
  {
    return extents_;
  }

  inline size_t size() noexcept
  {
    if(isArray())
    {
      return extents()[0];
    }
    else
    {
      return sizeBytes();
    }
    return 0;
  }

private:
  inline Variable* variable();
  lldb::SBType     type_;
  lldb::SBType     elementType_;
  QList<int>       extents_;
};