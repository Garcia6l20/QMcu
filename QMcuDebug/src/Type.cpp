#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/Type.hpp>

using namespace lldb;

Type::Type(SBType type, Variable* parent) : QObject(parent), type_(type)
{
}

Variable* Type::variable()
{
  return static_cast<Variable*>(parent());
}

QString Type::name()
{
  return type_.GetName();
}
