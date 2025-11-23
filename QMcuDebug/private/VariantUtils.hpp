#pragma once

#include <QVariant>

template <typename... AllowedTypes>
decltype(auto) qVisitSome(const QVariant& variant, auto&& visitor)
{
  return [&]<size_t... I>(std::index_sequence<I...>)
  {
    return (
        [&]<size_t II>
        {
          if(variant.typeId() == QMetaType::fromType<AllowedTypes...[II]>().id())
          {
            std::forward<decltype(visitor)>(visitor)(variant.value<AllowedTypes...[II]>());
            return true;
          }
          return false;
        }.template operator()<I>() or
        ...);
  }(std::index_sequence_for<AllowedTypes...>());
}

template <template <typename> typename Container, typename... AllowedTypes>
decltype(auto) qVisitSomeContainer(const QVariant& variant, auto&& visitor)
{
  return qVisitSome<Container<AllowedTypes>...>(variant, std::forward<decltype(visitor)>(visitor));
}
