#include <QMcu/Debug/AutoScale.hpp>
#include <QMcu/Debug/BufferPlotProvider.hpp>
#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/StLinkProbe.hpp>

#include <Logging.hpp>
#include <VariantUtils.hpp>

#include <QtGraphs/QValueAxis>

#include <QTimer>

#include <magic_enum/magic_enum.hpp>

BufferPlotProvider::BufferPlotProvider(QObject* parent) : AbstractVariablePlotDataProvider(parent)
{
}

void BufferPlotProvider::onValueChanged()
{
  if(not mappedData_.empty())
  {
    const auto& var = proxy()->value();
    qVisitSomeContainer<QList, //
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
                        double>(
        var,
        [this]<typename T>(QList<T> const& v) mutable
        { memcpy(mappedData_.data(), v.constData(), mappedData_.size_bytes()); });
  }
  dataChanged();
}

void BufferPlotProvider::onValueUnChanged()
{
}

bool BufferPlotProvider::initializePlotContext(PlotContext& ctx)
{
  if(auto p = proxy(); not p)
  {
    qWarning(lcWatcher) << "No proxy attached !";
    return false;
  }
  else
  {
    auto const* v = p->variable();
    if(v == nullptr)
    {
      return false;
    }

    const auto& var = proxy()->value();
    const bool  ok  = qVisitSomeContainer<QList, //
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
                                          double>(
        var,
        [this]<typename T>(QList<T> const& v) mutable
        { mappedData_ = std::as_writable_bytes(createMappedStorageBuffer<T>(v.size())); });
    if(not ok)
    {
      return false;
      // qFatal(lcWatcher) << "cannot handle given type:" << var.typeName();
    }

    std::ranges::fill(mappedData_, std::byte(0));
    return true;
  }
}

BufferPlotProvider::UpdateRange BufferPlotProvider::update(PlotContext& ctx)
{
  return ctx.vbo.full_range();
}