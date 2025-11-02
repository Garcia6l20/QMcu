#include <QMcu/Debug/AutoScale.hpp>
#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/ScrollPlotProvider.hpp>
#include <QMcu/Debug/StLinkProbe.hpp>

#include <Logging.hpp>

#include <QtCharts/QValueAxis>

#include <QTimer>

#include <magic_enum/magic_enum.hpp>

ScrollPlotProvider::ScrollPlotProvider(QObject* parent) : AbstractVariablePlotDataProvider(parent)
{
}

void ScrollPlotProvider::setSampleCount(int count)
{
  if(count != sampleCount_)
  {
    sampleCount_ = count;
    emit sampleCountChanged(sampleCount_);
  }
}

void ScrollPlotProvider::pushLastValue()
{
  if(mappedData_.empty())
  {
    return;
  }

  glsl::visitQtType(QMetaType::Type(lastValue_.typeId()),
                    [&]<typename T>
                    {
                      auto       data  = std::span(reinterpret_cast<T*>(mappedData_.data()),
                                            mappedData_.size_bytes() / sizeof(T));
                      const auto value = lastValue_.value<T>();
                      readIndex_       = currentOffset_;

                      const auto index           = (sampleCount_ + currentOffset_) % sampleCount_;
                      data[index]                = value;
                      data[index + sampleCount_] = value;

                      ++currentOffset_;
                      if(currentOffset_ >= sampleCount_)
                      {
                        currentOffset_ = 0;
                      }

                      dataChanged();
                    });
}

void ScrollPlotProvider::onValueChanged()
{
  lastValue_ = proxy()->value();
  pushLastValue();
}

void ScrollPlotProvider::onValueUnChanged()
{
  pushLastValue();
}

bool ScrollPlotProvider::initializePlotContext(PlotContext& ctx)
{
  if(auto p = proxy(); not p)
  {
    qWarning(lcWatcher) << "No proxy attached !";
    return false;
  }
  else
  {
    auto val = p->value();
    auto tid = QMetaType::Type(val.typeId());
    if(tid == QMetaType::Type::UnknownType)
    {
      return false;
    }
    mappedData_ = createMappedStorageBuffer(QMetaType::Type(val.typeId()), sampleCount_ * 2);
    std::ranges::fill(mappedData_, std::byte(0));
    return true;
  }
}

ScrollPlotProvider::UpdateRange ScrollPlotProvider::update(PlotContext& ctx)
{
  auto val = proxy()->value();
  return glsl::visitQtType(QMetaType::Type(val.typeId()),
                           [&]<typename T>
                           {
                             const auto data = std::span(reinterpret_cast<T*>(mappedData_.data()),
                                                         mappedData_.size_bytes() / sizeof(T));
                             return std::as_bytes(data.subspan(currentOffset_, sampleCount_));
                           });
}