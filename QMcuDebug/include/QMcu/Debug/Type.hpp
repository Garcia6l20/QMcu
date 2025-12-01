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
  Q_PROPERTY(Kind kind READ kind CONSTANT)

public:
  enum class KindBits : uint32_t
  {
    Invalid           = (0u),
    Array             = (1u << 0),
    BlockPointer      = (1u << 1),
    Builtin           = (1u << 2),
    Class             = (1u << 3),
    ComplexFloat      = (1u << 4),
    ComplexInteger    = (1u << 5),
    Enumeration       = (1u << 6),
    Function          = (1u << 7),
    MemberPointer     = (1u << 8),
    ObjCObject        = (1u << 9),
    ObjCInterface     = (1u << 10),
    ObjCObjectPointer = (1u << 11),
    Pointer           = (1u << 12),
    Reference         = (1u << 13),
    Struct            = (1u << 14),
    Typedef           = (1u << 15),
    Union             = (1u << 16),
    Vector            = (1u << 17),
    // Define the last type class as the MSBit of a 32 bit value
    Other = (1u << 31),
    // Define a mask that can be used for any type when finding types
    Any = (0xffffffffu)
  };
  Q_DECLARE_FLAGS(Kind, KindBits);
  Q_FLAG(Kind)

  Type(lldb::SBType type, Variable* parent);
  virtual ~Type() = default;

  QString name();

  inline Kind kind() const noexcept
  {
    return kind_;
  }

  inline bool isBasic()
  {
    return canonicalBasicType() != lldb::eBasicTypeInvalid;
  }

  inline lldb::BasicType canonicalBasicType()
  {
    if(isEnum())
    {
      return elementType_.GetCanonicalType().GetEnumerationIntegerType().GetBasicType();
    }
    return type_.GetCanonicalType().GetBasicType();
  }

  inline bool isArray()
  {
    return (kind_ & KindBits::Array) != 0;
  }

  inline bool isEnum()
  {
    return (kind_ & KindBits::Enumeration) != 0;
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
  Kind             kind_ = KindBits::Invalid;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Type::Kind)
