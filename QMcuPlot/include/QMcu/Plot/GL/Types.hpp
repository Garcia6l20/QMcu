#pragma once

#include <QMetaType>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions_4_5_Core>

#include <GL/glu.h>

template <typename T>
concept always_false_c = false;

namespace glsl
{
struct TypeId
{
  QMetaType::Type qt;
  GLenum          gl;
  size_t          size;
};

#define QPLOT_BASIC_TYPE_MAP(X)                     \
  X(float, QMetaType::Float, GL_FLOAT)              \
  X(double, QMetaType::Double, GL_DOUBLE)           \
  X(int32_t, QMetaType::Int, GL_INT)                \
  X(uint32_t, QMetaType::UInt, GL_UNSIGNED_INT)     \
  X(int16_t, QMetaType::Short, GL_SHORT)            \
  X(uint16_t, QMetaType::UShort, GL_UNSIGNED_SHORT) \
  X(int8_t, QMetaType::Char, GL_BYTE)               \
  X(uint8_t, QMetaType::UChar, GL_UNSIGNED_BYTE)

template <typename Visitor>
static constexpr auto visitQtType(QMetaType::Type qtType, Visitor&& fn) noexcept
{
  if(false)
  {
  }
#define X(__type, __qt_type, __gl_type)                             \
  else if(qtType == __qt_type)                                      \
  {                                                                 \
    return std::forward<Visitor>(fn).template operator()<__type>(); \
  }
  QPLOT_BASIC_TYPE_MAP(X)
#undef X
  else
  {
    std::abort();
  }
}

template <typename T> static constexpr TypeId typeIdOf() noexcept
{
  using Type = std::remove_cv_t<T>;
  if constexpr(false)
  {
  }
#define X(__type, __qt_type, __gl_type)            \
  else if constexpr(std::same_as<Type, __type>)    \
  {                                                \
    return {__qt_type, __gl_type, sizeof(__type)}; \
  }
  QPLOT_BASIC_TYPE_MAP(X)
#undef X
  else
  {
    static_assert(always_false_c<T>, "Unknown type");
  }
}

inline std::string_view checkError() noexcept
{
  if(const auto err = glGetError(); err != GL_NO_ERROR)
  {
    return reinterpret_cast<const char*>(gluErrorString(err));
  }
  return {};
}
} // namespace glsl