#pragma once

#include <QMetaType>

template <typename T>
concept always_false_c = false;

namespace qplot
{
struct TypeId
{
  QMetaType::Type qt;
  size_t          size;
};

#define QPLOT_BASIC_TYPE_MAP(X)     \
  X(float, QMetaType::Float)        \
  X(double, QMetaType::Double)      \
  X(int32_t, QMetaType::Int)        \
  X(uint32_t, QMetaType::UInt)      \
  X(int64_t, QMetaType::LongLong)   \
  X(uint64_t, QMetaType::ULongLong) \
  X(int16_t, QMetaType::Short)      \
  X(uint16_t, QMetaType::UShort)    \
  X(int8_t, QMetaType::Char)        \
  X(uint8_t, QMetaType::UChar)

template <typename Visitor>
static constexpr auto visitQtType(QMetaType::Type qtType, Visitor&& fn) noexcept
{
  if(false)
  {
  }
#define X(__type, __qt_type)                                        \
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
#define X(__type, __qt_type)                    \
  else if constexpr(std::same_as<Type, __type>) \
  {                                             \
    return {__qt_type, sizeof(__type)};         \
  }
  QPLOT_BASIC_TYPE_MAP(X)
#undef X
  else
  {
    static_assert(always_false_c<T>, "Unknown type");
  }
}

} // namespace qplot