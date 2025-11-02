#include <QMcu/Debug/BufferRecorder.hpp>

#include <Logging.hpp>

BufferRecorder::BufferRecorder(QObject* parent) : AbstractVariableRecorder{parent}
{
  QTimer::singleShot(0,
                     [this]
                     {
                       if(not series_)
                       {
                         qCritical(lcWatcher) << "no series attached";
                         return;
                       }
                     });
}

void BufferRecorder::setSeries(QLineSeries* series)
{
  if(series != series_)
  {
    series_ = series;
    if(series_->name().isEmpty() and proxy() != nullptr)
    {
      series_->setName(proxy()->name());
    }
    emit seriesChanged();
  }
}

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

void BufferRecorder::onValueChanged()
{
  //   bool        ok  = false;
  const auto& var = proxy()->value();

  const bool ok = qVisitSomeContainer<QList, //
                                      int,
                                      int8_t,
                                      uint8_t, //
                                      int16_t,
                                      uint16_t, //
                                      int32_t,
                                      uint32_t, //
                                      int64_t,
                                      uint64_t, //
                                      float,
                                      double>(var,
                                              [this]<typename T>(QList<T> const& v) mutable
                                              {
                                                if(v.size() != series_->count())
                                                {
                                                  series_->clear();

                                                  for(int ii = 0; ii < v.size(); ++ii)
                                                  {
                                                    series_->append(ii, v.at(ii));
                                                  }
                                                }
                                                else
                                                {
                                                  for(int ii = 0; ii < v.size(); ++ii)
                                                  {
                                                    series_->replace(ii, ii, v.at(ii));
                                                  }
                                                }
                                              });
  if(not ok)
  {
    qFatal(lcWatcher) << "cannot handle given type:" << var.typeName();
  }
}