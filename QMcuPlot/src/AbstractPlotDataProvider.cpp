#include <QMcu/Plot/AbstractPlotDataProvider.hpp>
#include <QMcu/Plot/AbstractPlotSeries.hpp>

void* AbstractPlotDataProvider::createMappedBuffer(glsl::TypeId tid,
                                                   size_t     count,
                                                   GLuint     bufferType,
                                                   GLuint     binding)
{
  return series_->createMappedBuffer(tid, count, bufferType, binding);
}
