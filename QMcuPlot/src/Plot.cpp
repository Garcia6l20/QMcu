#include <QMcu/Plot/AbstractPlotSeries.hpp>
#include <QMcu/Plot/Plot.hpp>
#include <QMcu/Plot/PlotGrid.hpp>
#include <QMcu/Plot/PlotScene.hpp>

#include <Logging.hpp>

#include <QQuickWindow>
#include <QTimer>
#include <QValueAxis>

#include <ranges>

Plot::Plot(QQuickItem* parent) : QQuickItem(parent), grid_(new PlotGrid(this))
{
  // setMirrorVertically(true);
  // setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true);
  setFlag(ItemHasContents, true);
  QTimer::singleShot(0, [this] { updateAxes(); });
}

Plot::~Plot() = default;

QSGNode* Plot::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
  PlotScene* node = static_cast<PlotScene*>(old);
  if(not node)
  {
    node = renderer_ = new PlotScene(window());
    renderer_->setBorder(border_);
    renderer_->setRadius(radius_);
    renderer_->addRenderer(grid_);
    for(auto* s : series_)
    {
      renderer_->addRenderer(s);
    }
  }
  node->setBoundingRect(mapRectToScene(boundingRect()));
  return node;
}

void Plot::componentComplete()
{
  QQuickItem::componentComplete();

  for(auto const& series :
      children()
          | std::views::filter(
              [this](QObject* item)
              { return item != grid_ && qobject_cast<AbstractPlotSeries*>(item) != nullptr; })
          | std::views::transform([](QObject* item)
                                  { return qobject_cast<AbstractPlotSeries*>(item); }))
  {
    addSeries(series);
  }
}

void Plot::itemChange(ItemChange change, const ItemChangeData& data)
{
  QQuickItem::itemChange(change, data);
  switch(change)
  {
    case ItemChange::ItemChildAddedChange:
      if(auto* s = qobject_cast<AbstractPlotSeries*>(data.item))
      {
        addSeries(s);
      }
      break;
    case ItemChange::ItemChildRemovedChange:
      if(auto* s = qobject_cast<AbstractPlotSeries*>(data.item))
      {
        removeSeries(s);
      }
      break;
    default:
      break;
  }
}

void Plot::draw()
{
  for(auto* s : series_)
  {
    s->setDirty();
  }
  update();
}

void Plot::updateAxes()
{
  for(auto* s : series_)
  {
    {
      auto*&      ax   = axisX_;
      auto* const s_ax = s->axisX();
      if(ax != nullptr)
      {
        if(s_ax == nullptr)
        {
          s->setAxisX(ax);
        }
        else if(s_ax != ax)
        {
          qFatal(lcPlot) << "Series" << s->name() << "has invalid X axis";
        }
      }
      else if(s_ax != nullptr)
      {
        ax = s_ax;
      }
    }

    {
      auto*&      ax   = axisY_;
      auto* const s_ax = s->axisY();
      if(ax != nullptr)
      {
        if(s_ax == nullptr)
        {
          s->setAxisY(ax);
        }
        else if(s_ax != ax)
        {
          qFatal(lcPlot) << "Series" << s->name() << "has invalid Y axis";
        }
      }
      else if(s_ax != nullptr)
      {
        ax = s_ax;
      }
    }

    connect(s, &AbstractPlotSeries::transformsChanged, this, &Plot::transformsChanged);
  }
  emit axesChanged(axisX_, axisY_);
}

void Plot::setAxisX(QAbstractAxis* ax)
{
  if(ax != axisX_)
  {
    axisX_ = ax;
    updateAxes();
  }
}

void Plot::setAxisY(QAbstractAxis* ax)
{
  if(ax != axisY_)
  {
    axisY_ = ax;
    updateAxes();
  }
}

void Plot::addSeries(AbstractPlotSeries* s)
{
  series_.append(s);
  PlotPointInfo ppi;
  ppi.series = s;
  pointInfos_.append(ppi);
  if(renderer_)
  {
    renderer_->addRenderer(s);
  }
  updateAxes();
  update(); // request redraw
  emit seriesChanged();
  emit pointInfosChanged(pointInfos_);
}

void Plot::removeSeries(AbstractPlotSeries* s)
{
  const auto index = series_.indexOf(s);
  series_.removeAt(index);
  pointInfos_.removeAt(index);
  if(renderer_)
  {
    renderer_->removeRenderer(s);
  }
  updateAxes();
  update(); // request redraw
  emit seriesChanged();
  emit pointInfosChanged(pointInfos_);
}

void Plot::pan(float x, float y)
{
  const auto toNdcXScale = toNdc_(0, 0);
  const auto toNdcYScale = toNdc_(1, 1);

  const auto ndcPan = QPointF{x * toNdcXScale, y * toNdcYScale};
  // qDebug(lcPlot) << "ndcPan:" << ndcPan;

  for(auto* s : series_)
  {
    auto&      ctx               = s->context();
    const auto itransform        = ctx.view.transform.inverted();
    const auto toZoomedNdcXScale = itransform(0, 0);
    const auto toZoomedNdcYScale = itransform(1, 1);
    const auto zpan = QPointF{ndcPan.x() * toZoomedNdcXScale, ndcPan.y() * toZoomedNdcYScale};
    // qDebug(lcPlot) << "zpan:" << zpan;
    ctx.view.transform.translate(zpan.x(), zpan.y());
    emit s->transformsChanged();
  }
  update();
}

void Plot::panX(float x)
{
  pan(x, 0);
}

void Plot::panY(float y)
{
  pan(0, y);
}

void Plot::zoomIn(const QRectF& rect)
{
  auto const& br      = boundingRect();
  auto const  brNdc   = toNdc_.mapRect(br);
  auto const  rectNdc = toNdc_.mapRect(rect);
  const auto  zoomM   = rectTransform(rectNdc, brNdc);

  for(auto* s : series_)
  {
    if(s->dataProvider())
    {
      auto& ctx          = s->context();
      ctx.view.transform = zoomM * ctx.view.transform;
      emit s->transformsChanged();
    }
  }
  update();
}

void Plot::zoomX(float x, float xCenter)
{
  auto const ndcCenter = toNdc_.map(QPointF{xCenter, 0});
  for(auto* s : series_)
  {
    auto&      ctx = s->context();
    const auto nc  = ctx.view.transform.inverted().map(ndcCenter).x();
    ctx.view.transform.translate(nc, 0);
    ctx.view.transform.scale(x, 1);
    ctx.view.transform.translate(-nc, 0);
    // qDebug() << xCenter << "(" << ndcCenter.x() << ")" << "->" << nc;
    emit s->transformsChanged();
  }
  update();
}

void Plot::zoomY(float y, float yCenter)
{
  auto const ndcCenter = toNdc_.map(QPointF{0, yCenter});
  for(auto* s : series_)
  {
    auto&      ctx = s->context();
    const auto nc  = ctx.view.transform.inverted().map(ndcCenter).y();
    ctx.view.transform.translate(0, nc);
    ctx.view.transform.scale(1, y);
    ctx.view.transform.translate(0, -nc);
    // qDebug() << yCenter << "(" << ndcCenter.y() << ")" << "->" << nc;
    emit s->transformsChanged();
  }
  update();
}

void Plot::zoomReset()
{
  for(auto* s : series_)
  {
    auto& ctx = s->context();
    ctx.view.transform.setToIdentity();
    emit s->transformsChanged();
  }
  update();
}

void Plot::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
  fromNdc_.setToIdentity();
  fromNdc_.viewport(newGeometry);
  fromNdc_.scale(1.0, -1.0); // flip Y axis
  toNdc_ = fromNdc_.inverted();
}

QList<PlotPointInfo> Plot::unitPointsAt(QPointF const& pt)
{
  QList<PlotPointInfo> result;

  const auto ndc = toNdc_.map(pt);

  for(auto* s : series_)
  {
    if(s->dataProvider())
    {
      auto&         ctx        = s->context();
      const auto    ndc_zoomed = ctx.view.transform.inverted().map(ndc);
      PlotPointInfo ppi;
      ppi.series          = s;
      ppi.mouseLocalPoint = pt;
      // ppi.mouseDataPoint  = ctx.data.fromNdc.map(ndc_zoomed);
      ppi.mouseDataPoint = ctx.unit.ndcToData.map(ndc_zoomed);
      ppi.mouseUnitPoint = ctx.unit.fromData.map(ppi.mouseDataPoint);
      result.append(ppi);
    }
  }
  return result;
}

QList<PlotPointInfo> Plot::valuesAt(QPointF const& pt)
{
  QList<PlotPointInfo> result = pointInfos_;
  updatePointInfos(pt, result);
  return result;
}

void Plot::autoScale(int margin)
{
  std::optional<float> topNdc;
  std::optional<float> bottomNdc;
  for(auto* s : series_)
  {
    if(s->dataProvider())
    {
      auto& ctx = s->context();
      ctx.visitCurrentData(
          [&]<typename T>(std::span<T> data)
          {
            const auto min    = std::ranges::min(data);
            const auto max    = std::ranges::max(data);
            const auto minNdc = ctx.unit.dataToNdc.map(QPointF{0, qreal(min)}).y();
            const auto maxNdc = ctx.unit.dataToNdc.map(QPointF{0, qreal(max)}).y();
            if(not bottomNdc.has_value() or minNdc < bottomNdc.value())
            {
              bottomNdc = minNdc;
            }
            if(not topNdc.has_value() or maxNdc > topNdc.value())
            {
              topNdc = maxNdc;
            }
          });
    }
  }

  QRectF ndcZoom;
  ndcZoom.setLeft(-1);
  ndcZoom.setRight(1);
  ndcZoom.setBottom(bottomNdc.value_or(-1));
  ndcZoom.setTop(topNdc.value_or(1));

  for(auto* s : series_)
  {
    auto& ctx = s->context();
    ctx.view.transform.setToIdentity();
  }

  zoomIn(fromNdc_.mapRect(ndcZoom).adjusted(0, -margin, 0, margin));
}

#define QMCU_PLOT_POINTINFO_MODE_LERP

void Plot::updatePointInfos(QPointF const& pt, QList<PlotPointInfo>& pis)
{
  const auto ndc = toNdc_.map(pt);
  // qDebug(lcPlot) << pt << "=>" << ndc;

  for(auto& ppi : pis)
  {
    auto* s = ppi.series;
    if(s->dataProvider())
    {
      auto& ctx = s->context();

      const auto ndc_zoomed = ctx.view.transform.inverted().map(ndc);
      // qDebug(lcPlot) << "ndc_zoomed = " << ndc_zoomed;

      ppi.mouseLocalPoint = pt;
      // ppi.mouseDataPoint  = ctx.data.fromNdc.map(ndc_zoomed);
      ppi.mouseDataPoint = ctx.unit.ndcToData.map(ndc_zoomed);
      ppi.mouseUnitPoint = ctx.unit.fromData.map(ppi.mouseDataPoint);
      ctx.visitCurrentData(
          [&]<typename T>(std::span<T> data)
          {
            if(data.empty())
            {
              return;
            }
            const auto prevMouseDataX = ppi.mouseDataPoint.x();
            const auto index          = std::clamp(int(prevMouseDataX), 0, int(data.size() - 1));
#ifdef QMCU_PLOT_POINTINFO_MODE_LERP
            const auto next  = std::min(index + 1, int(data.size() - 1));
            const auto alpha = prevMouseDataX - int(prevMouseDataX);
            const auto value = std::lerp(double(data[index]), double(data[next]), alpha);
#else // basic mode: previous neighbor
            const auto value = data[index];
#endif

            ppi.seriesDataPoint             = {ppi.mouseDataPoint.x(), double(value)};
            ppi.seriesUnitPoint             = ctx.unit.fromData.map(ppi.seriesDataPoint);
            const auto seriesZoomedNdcPoint = ctx.unit.toNdc.map(ppi.seriesUnitPoint);
            const auto seriesNdcPoint       = ctx.view.transform.map(seriesZoomedNdcPoint);
            ppi.seriesLocalPoint            = fromNdc_.map(seriesNdcPoint);
            // qDebug(lcPlot).nospace() << " qt=" << pt << " ndc=" << ndc
            //                    << " ndc_zoomed=" << ndc_zoomed << " => data=" <<
            //                    ppi.seriesDataPoint
            //                    << " => unit=" << ppi.seriesUnitPoint;
          });
    }
  }
}

void Plot::hoverEnterEvent(QHoverEvent* event)
{
  // qDebug(lcPlot) << "Plot::hoverEnterEvent" << event->position();
}

void Plot::hoverMoveEvent(QHoverEvent* event)
{
  // qDebug(lcPlot) << "Plot::hoverMoveEvent" << event->position();
  updatePointInfos(event->position(), pointInfos_);
  emit pointInfosChanged(pointInfos_);
}

void Plot::hoverLeaveEvent(QHoverEvent* event)
{
  // qDebug(lcPlot) << "Plot::hoverLeaveEvent" << event->position();
}