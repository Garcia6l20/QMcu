#include <QMcu/Plot/AbstractPlotDataProvider.hpp>
#include <QMcu/Plot/AbstractPlotSeries.hpp>

void* AbstractPlotDataProvider::createMappedBuffer(qplot::TypeId           tid,
                                                   size_t                  count,
                                                   vk::BufferUsageFlagBits usage)
{
  return series_->createMappedBuffer(tid, count, usage);
}
