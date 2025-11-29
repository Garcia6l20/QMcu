#include <QMcu/Plot/AbstractPlotSeries.hpp>

#include <QValueAxis>

#include <Logging.hpp>

uint32_t AbstractPlotSeries::s_instanceCount_ = 0;

AbstractPlotSeries::AbstractPlotSeries(QObject* parent)
    : PlotSceneItem{parent}, id_(++s_instanceCount_)
{
}

void* AbstractPlotSeries::createMappedBuffer(qplot::TypeId           tid,
                                             size_t                  count,
                                             vk::BufferUsageFlagBits usage)
{
  ctx_.data.type  = tid.qt;
  ctx_.vbo.stride = ctx_.vbo.elem_size = tid.size;

  const size_t size_bytes = count * ctx_.vbo.stride;

  auto& vk = vkContext();

  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size  = aligned(size_bytes, vk.physDevProps.limits.minStorageBufferOffsetAlignment);
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  ctx_.vbo._buffer = vk.dev.createBuffer(bufferInfo);

  vk::MemoryRequirements memReq = vk.dev.getBufferMemoryRequirements(ctx_.vbo._buffer);

  auto const memTypeIndex =
      vk.findMemoryTypeIndex(memReq,
                             vk::MemoryPropertyFlagBits::eHostVisible
                                 | vk::MemoryPropertyFlagBits::eHostCoherent // persistent/coherent
      );
  if(memTypeIndex == UINT32_MAX)
    qFatal("Failed to find host visible and coherent memory type");

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.setAllocationSize(memReq.size);
  allocInfo.setMemoryTypeIndex(memTypeIndex);
  ctx_.vbo._bufferMem = vk.dev.allocateMemory(allocInfo);
  vk.dev.bindBufferMemory(ctx_.vbo._buffer, ctx_.vbo._bufferMem, 0);

  void* mappedPtr = vk.dev.mapMemory(ctx_.vbo._bufferMem, 0, bufferInfo.size);
  ctx_.vbo._range = {reinterpret_cast<std::byte*>(mappedPtr), size_bytes};
  return mappedPtr;
}

void AbstractPlotSeries::doReleaseResources()
{
  ctx_.vbo.releaseResources(vkContext().dev);
}

void AbstractPlotSeries::setAxisX(QAbstractAxis* xAxis)
{
  if(xAxis != axisX_)
  {
    axisX_ = xAxis;
    if(auto* ax = qobject_cast<QValueAxis*>(axisX_))
    {
      connect(ax, &QValueAxis::minChanged, this, &AbstractPlotSeries::updateTransforms);
      connect(ax, &QValueAxis::maxChanged, this, &AbstractPlotSeries::updateTransforms);
      updateTransforms();
    }
    else
    {
      qFatal(lcPlot) << "Unhandled axis:" << axisX_->metaObject()->className();
    }
    emit axisXChanged(axisX_);
  }
}

void AbstractPlotSeries::setAxisY(QAbstractAxis* yAxis)
{
  if(yAxis != axisY_)
  {
    axisY_ = yAxis;
    if(auto* ax = qobject_cast<QValueAxis*>(axisY_))
    {
      connect(ax, &QValueAxis::minChanged, this, &AbstractPlotSeries::updateTransforms);
      connect(ax, &QValueAxis::maxChanged, this, &AbstractPlotSeries::updateTransforms);
      updateTransforms();
    }
    else
    {
      qFatal(lcPlot) << "Unhandled axis:" << axisY_->metaObject()->className();
    }
    emit axisYChanged(axisY_);
  }
}

void AbstractPlotSeries::updateTransforms()
{
  auto& unitToData = ctx_.unit.toData;

  QRectF dataBounds{0, 0, 1, 1};
  QRectF unitBounds{0, 0, 1, 1};

  if(axisX_ != nullptr)
  {
    auto const* const axis = qobject_cast<QValueAxis*>(axisX_);
    dataBounds.setLeft(axis->min());
    dataBounds.setRight(axis->max());
  }
  if(axisY_ != nullptr)
  {
    auto const* const axis = qobject_cast<QValueAxis*>(axisY_);
    dataBounds.setBottom(axis->min());
    dataBounds.setTop(axis->max());
  }
  unitBounds = dataBounds; // TODO only one is required
  unitToData.setToIdentity();
  unitToData = rectTransform(unitBounds, dataBounds);

  auto& unitFromData = ctx_.unit.fromData = unitToData.inverted();

  // qDebug().nospace() << "unitFromData(" << dataBounds.bottomLeft() << ") => "
  //                    << unitFromData.map(dataBounds.bottomLeft());
  // qDebug().nospace() << "unitFromData(" << dataBounds.topRight() << ") => "
  //                    << unitFromData.map(dataBounds.topRight());

  // qDebug().nospace() << "unitToData(" << unitBounds.bottomLeft() << ") => "
  //                    << unitToData.map(unitBounds.bottomLeft());
  // qDebug().nospace() << "unitToData(" << unitBounds.topRight() << ") => "
  //                    << unitToData.map(unitBounds.topRight());

  auto& ndcToUnit = ctx_.unit.fromNdc;
  ndcToUnit.setToIdentity();
  ndcToUnit.viewport(unitBounds.left(),
                     unitBounds.bottom(),
                     (unitBounds.right() - unitBounds.left()) - 1,
                     unitBounds.top() - unitBounds.bottom());
  auto& unitToNdc = ctx_.unit.toNdc = ndcToUnit.inverted();

  auto& ndcToData = ctx_.unit.ndcToData = unitToData * ndcToUnit;
  auto& dataToNdc = ctx_.unit.dataToNdc = ndcToData.inverted();

  // QRectF ndcBounds;
  // ndcBounds.setLeft(-1);
  // ndcBounds.setRight(1);
  // ndcBounds.setBottom(-1);
  // ndcBounds.setTop(1);

  // qDebug().nospace() << "dataToNdc(" << dataBounds.bottomLeft() << ") => "
  //                    << dataToNdc.map(dataBounds.bottomLeft());
  // qDebug().nospace() << "dataToNdc(" << dataBounds.topRight() << ") => "
  //                    << dataToNdc.map(dataBounds.topRight());

  // qDebug().nospace() << "ndcToData(" << ndcBounds.bottomLeft() << ") => "
  //                    << ndcToData.map(ndcBounds.bottomLeft());
  // qDebug().nospace() << "ndcToData(" << ndcBounds.topRight() << ") => "
  //                    << ndcToData.map(ndcBounds.topRight());

  emit transformsChanged();
}