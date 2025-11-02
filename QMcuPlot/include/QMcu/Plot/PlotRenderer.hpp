#pragma once

#include <QMcu/Plot/AbstractPlotSeries.hpp>
#include <QMcu/Plot/PlotContext.hpp>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLFunctions_4_5_Core>

#include <QQuickFramebufferObject>

class Plot;

class PlotRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions_4_5_Core
{
public:
  PlotRenderer(Plot* plot);
  virtual ~PlotRenderer() = default;

  void                      render() override;
  QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;
  void                      synchronize(QQuickFramebufferObject* item) override;

private:
  Plot*                plot_;
  QSize                viewport_;
  static QElapsedTimer sTimer_;
};
