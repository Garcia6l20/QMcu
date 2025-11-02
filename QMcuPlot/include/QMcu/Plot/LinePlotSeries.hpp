#pragma once

#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QtQmlIntegration>

#include <QMcu/Plot/AbstractPlotDataProvider.hpp>
#include <QMcu/Plot/AbstractPlotSeries.hpp>
#include <QMcu/Plot/PlotContext.hpp>

#include <QColor>

class LinePlotSeries : public AbstractPlotSeries
{
  friend PlotContext;
  friend AbstractPlotDataProvider;

  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
  Q_PROPERTY(float thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
  Q_PROPERTY(float glow READ glow WRITE setGlow NOTIFY glowChanged)

  Q_PROPERTY(LineStyle lineStyle READ lineStyle WRITE setLineStyle NOTIFY lineStyleChanged)

public:
  enum LineStyle
  {
    Basic,
    Halo,
  };
  Q_ENUM(LineStyle)

  explicit LinePlotSeries(QObject* parent = nullptr);
  virtual ~LinePlotSeries();

  QColor const& lineColor() const noexcept
  {
    return lineColor_;
  }
  float thickness() const noexcept
  {
    return thickness_;
  }
  float glow() const noexcept
  {
    return glow_;
  }
  LineStyle lineStyle() const noexcept
  {
    return lineStyle_;
  }

public slots:

  void setLineColor(QColor const& color)
  {
    if(color != lineColor_)
    {
      lineColor_ = color;
      emit lineColorChanged();
    }
  }

  void setThickness(float thickness)
  {
    if(thickness != thickness_)
    {
      thickness_ = thickness;
      emit thicknessChanged(thickness_);
    }
  }

  void setGlow(float glow)
  {
    if(glow != glow_)
    {
      glow_ = glow;
      emit glowChanged(glow_);
    }
  }

  void setLineStyle(LineStyle lineStyle)
  {
    if(lineStyle != lineStyle_)
    {
      lineStyle_ = lineStyle;
      emit lineStyleChanged(lineStyle_);
    }
  }

signals:
  void lineColorChanged();
  void thicknessChanged(float);
  void glowChanged(float);

  void lineStyleChanged(LineStyle);

protected:
  bool allocateGL(QSize const& viewport) final;
  void draw() final;

private:
  std::string generateShaderSource(PlotContext& ctx);

  std::unique_ptr<QOpenGLShaderProgram> program_;
  QColor                                lineColor_ = Qt::red;
  float                                 thickness_ = 7 / 100.0f;
  float                                 glow_      = 1 / 100.0f;
  LineStyle                             lineStyle_ = LineStyle::Basic;
};
