#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/Type.hpp>

using namespace lldb;

Type::Type(SBType type, Variable* parent)
    : QObject(parent), type_(type), kind_(Kind(type_.GetTypeClass()))
{
  if(isArray())
  {
    SBType elemType = type_;
    while(elemType.IsArrayType())
    {
      auto       nextElement = elemType.GetArrayElementType();
      const auto extentSize  = elemType.GetByteSize() / nextElement.GetByteSize();
      extents_.append(static_cast<int>(extentSize));
      elemType = nextElement;
    }
    elementType_ = elemType;
  }
  else if(type_.IsTypedefType())
  {
    elementType_ = type_.GetTypedefedType();
    kind_        = Kind(elementType_.GetTypeClass());
  }
  else
  {
    elementType_ = type_;
  }
}

Variable* Type::variable()
{
  return static_cast<Variable*>(parent());
}

QString Type::name()
{
  return type_.GetName();
}
