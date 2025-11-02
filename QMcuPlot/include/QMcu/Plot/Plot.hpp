#pragma once

#include <QMcu/Plot/AbstractPlotDataProvider.hpp>
#include <QMcu/Plot/AbstractPlotSeries.hpp>
#include <QMcu/Plot/PlotContext.hpp>
#include <QMcu/Plot/PlotGrid.hpp>

#include <QOpenGLFunctions_4_5_Core>
#include <QQuickFramebufferObject>

#include <QQmlListProperty>
#include <QtQmlIntegration>

struct PlotPointInfo
{
  Q_GADGET
  QML_VALUE_TYPE(plotPointInfo)
  QML_UNCREATABLE("Created by Plot")

  Q_PROPERTY(QPointF seriesDataPoint MEMBER seriesDataPoint)
  Q_PROPERTY(QPointF mouseDataPoint MEMBER mouseDataPoint)
  Q_PROPERTY(QPointF seriesUnitPoint MEMBER seriesUnitPoint)
  Q_PROPERTY(QPointF mouseUnitPoint MEMBER mouseUnitPoint)
  Q_PROPERTY(QPointF seriesLocalPoint MEMBER seriesLocalPoint)
  Q_PROPERTY(QPointF mouseLocalPoint MEMBER mouseLocalPoint)

  Q_PROPERTY(AbstractPlotSeries* series MEMBER series)

public:
  PlotPointInfo()                                = default;
  PlotPointInfo(PlotPointInfo&&)                 = default;
  PlotPointInfo(const PlotPointInfo&)            = default;
  PlotPointInfo& operator=(PlotPointInfo&&)      = default;
  PlotPointInfo& operator=(const PlotPointInfo&) = default;

  QPointF seriesDataPoint;
  QPointF mouseDataPoint;
  QPointF seriesUnitPoint;
  QPointF mouseUnitPoint;
  QPointF seriesLocalPoint;
  QPointF mouseLocalPoint;

  AbstractPlotSeries* series = nullptr;
};
Q_DECLARE_METATYPE(PlotPointInfo)

class Plot : public QQuickFramebufferObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QQmlListProperty<AbstractPlotSeries> series READ series NOTIFY seriesChanged)
  Q_PROPERTY(PlotGrid* grid READ grid CONSTANT)

  Q_PROPERTY(QAbstractAxis* axisX READ axisX WRITE setAxisX NOTIFY axesChanged)
  Q_PROPERTY(QAbstractAxis* axisY READ axisY WRITE setAxisY NOTIFY axesChanged)

public:
  Plot(QQuickItem* parent = nullptr);
  ~Plot() override;

  Renderer* createRenderer() const override;

  Q_INVOKABLE void                           addSeries(AbstractPlotSeries* s);
  Q_INVOKABLE void                           removeSeries(AbstractPlotSeries* s);
  const QQmlListProperty<AbstractPlotSeries> series()
  {
    return QQmlListProperty<AbstractPlotSeries>(this, &series_);
  }

  QAbstractAxis* axisX() const noexcept
  {
    return axisX_;
  }
  QAbstractAxis* axisY() const noexcept
  {
    return axisY_;
  }

  PlotGrid* grid() noexcept
  {
    return grid_;
  }

public slots:
  void draw();

  void pan(float x, float y);
  void panX(float x);
  void panY(float y);

  void zoomIn(const QRectF& rect);
  void zoomX(float x, float xCenter);
  void zoomY(float y, float yCenter);

  void zoomReset();

  Q_INVOKABLE QList<PlotPointInfo> valuesAt(QPointF const& pt);
  Q_INVOKABLE QList<PlotPointInfo> unitPointsAt(QPointF const& pt);

  void setAxisX(QAbstractAxis*);
  void setAxisY(QAbstractAxis*);

  Q_INVOKABLE void autoScale(int margin = 16);

signals:
  void beforeRender();
  void afterRender();
  void seriesChanged();
  void axesChanged(QAbstractAxis* x, QAbstractAxis* y);
  void transformsChanged();

protected:
  static void appendSeries(QQmlListProperty<AbstractPlotSeries>* prop, AbstractPlotSeries* series)
  {
    static_cast<Plot*>(prop->object)->addSeries(series);
  }

  void componentComplete() override;
  void itemChange(ItemChange, const ItemChangeData&) override;

  void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

  void updateAxes();

private:
  friend PlotRenderer;

  PlotRenderer*              renderer_;
  PlotGrid*                  grid_;
  QList<AbstractPlotSeries*> series_;
  QAbstractAxis*             axisX_ = nullptr;
  QAbstractAxis*             axisY_ = nullptr;
  QMatrix4x4                 toNdc_;
  QMatrix4x4                 fromNdc_;
};
