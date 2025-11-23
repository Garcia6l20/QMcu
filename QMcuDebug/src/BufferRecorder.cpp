#include <QMcu/Debug/BufferRecorder.hpp>

#include <Logging.hpp>
#include <VariantUtils.hpp>

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