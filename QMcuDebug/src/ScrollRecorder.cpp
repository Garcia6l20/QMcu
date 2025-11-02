#include <QMcu/Debug/AutoScale.hpp>
#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/ScrollRecorder.hpp>
#include <QMcu/Debug/StLinkProbe.hpp>

#include <Logging.hpp>

#include <QtCharts/QValueAxis>

#include <QTimer>

#include <magic_enum/magic_enum.hpp>

ScrollRecorder::ScrollRecorder(QObject* parent) : AbstractVariableRecorder(parent)
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

void ScrollRecorder::setSeries(QLineSeries* series)
{
  if(series != series_ or (series_ != nullptr and series_->count() != sampleCount_))
  {
    series_ = series;
    if(series_->name().isEmpty() and proxy() != nullptr)
    {
      series_->setName(proxy()->name());
    }
    series_->clear();
    if(xMode_ == Index)
    {
      for(int ii = 0; ii < sampleCount_; ++ii)
      {
        series_->append(ii, 0);
      }
    }
    else
    {
      const qreal x = [mode = xMode_] -> qreal
      {
        switch(mode)
        {
          case XMode::MiliSeconds:
            return msTime();
          default:
          case XMode::Seconds:
            return sTime();
        }
      }();
      for(int ii = 0; ii < sampleCount_; ++ii)
      {
        series_->append(x, 0);
      }
    }
    emit seriesChanged();
  }
}

// bool ScrollRecorder::start()
// {
//   const bool ok = AbstractVariableRecorder::start();
//   if(ok)
//   {
//     series_->clear();
//     const auto  t0    = sTime();
//     const qreal value = factor_ * variable()->readAsReal();
//     for(int ii = 0; ii < sampleCount_; ++ii)
//     {
//       series_->append(t0, value);
//     }
//     emit seriesChanged();
//   }
//   return ok;
// }

void ScrollRecorder::setSampleCount(int count)
{
  if(count != sampleCount_)
  {
    sampleCount_ = count;
    if(series_)
    {
      setSeries(series_);
    }
    emit sampleCountChanged(sampleCount_);
  }
}

void ScrollRecorder::onValueChanged()
{
  bool        ok  = false;
  const auto& var = proxy()->value();
  const auto  r   = var.toDouble(&ok);
  if(not ok)
  {
    qFatal(lcWatcher) << variable()->name()
                      << "should be convertible to double, type:" << variable()->type()->name();
  }
  const qreal value = factor_ * r;
  // qDebug(lcWatcher) << value << var;

  if(xMode_ == Index)
  {
    size_t ii = 0;
    for(; ii < series_->count() - 1; ++ii)
    {
      series_->replace(ii, ii, series_->at(ii + 1).y());
    }
    series_->replace(ii, ii, value);
  }
  else
  {
    const qreal x = [mode = xMode_] -> qreal
    {
      switch(mode)
      {
        case XMode::MiliSeconds:
          return msTime();
        default:
        case XMode::Seconds:
          return sTime();
      }
    }();
    series_->remove(0);
    series_->append(x, value);
  }
}

void ScrollRecorder::onValueUnChanged()
{
  const auto t = sTime();
  series_->remove(0);
  const auto& prev_last = series_->at(series_->count() - 1);
  series_->append(t, prev_last.y());
}
