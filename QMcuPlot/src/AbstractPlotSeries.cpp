#include <QMcu/Plot/AbstractPlotSeries.hpp>

#include <QValueAxis>

#include <Logging.hpp>

uint32_t AbstractPlotSeries::s_instanceCount_ = 0;

AbstractPlotSeries::AbstractPlotSeries(QObject* parent)
    : BasicRenderer{parent}, id_(++s_instanceCount_)
{
}

void* AbstractPlotSeries::createMappedBuffer(glsl::TypeId tid,
                                             size_t       count,
                                             GLuint       bufferType,
                                             GLuint       binding)
{
  ctx_.data.type = tid.qt;

  ctx_.vbo._gl_type = tid.gl;
  ctx_.vbo.stride = ctx_.vbo.elem_size = tid.size;

  if(not isInitialized())
  {
    initializeOpenGLFunctions();
  }

  if(!ctx_.vbo._gl_handle)
  {
    glGenBuffers(1, &ctx_.vbo._gl_handle);
  }

  glBindBuffer(bufferType, ctx_.vbo._gl_handle);

  glBufferStorage(bufferType,
                  count * tid.size,
                  nullptr,
                  GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

  glBindBufferBase(bufferType, binding, ctx_.vbo._gl_handle);

  const size_t size_bytes = count * ctx_.vbo.stride;
  ctx_.vbo._range         = {reinterpret_cast<std::byte*>(glMapBufferRange(
                         bufferType,
                         0,
                         size_bytes,
                         GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)),
                             size_bytes};
  return ctx_.vbo._range.data();
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
                     unitBounds.right() - unitBounds.left(),
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