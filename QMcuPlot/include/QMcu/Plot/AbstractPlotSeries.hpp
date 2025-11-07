#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include <QMcu/Plot/AbstractPlotDataProvider.hpp>
#include <QMcu/Plot/PlotContext.hpp>
#include <QMcu/Plot/PlotSceneItem.hpp>

#include <QAbstractAxis>

class Plot;
class PlotScene;
class PlotContext;
class AbstractPlotDataProvider;

static inline QMatrix4x4 rectTransform(const QRectF& src, const QRectF& dst)
{
  // Compute scale factors
  const qreal sx = dst.width() / src.width();
  const qreal sy = dst.height() / src.height();

  // Compute translation
  const qreal tx = dst.x() - src.x() * sx;
  const qreal ty = dst.y() - src.y() * sy;

  QMatrix4x4 m;

  // Apply transform
  m.translate(tx, ty);
  m.scale(sx, sy);

  return m;
}

class AbstractPlotSeries : public PlotSceneItem
{
  friend Plot;
  friend PlotScene;
  friend PlotContext;
  friend AbstractPlotDataProvider;

  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("abstract element")

  Q_CLASSINFO("DefaultProperty", "dataProvider")
  Q_PROPERTY(AbstractPlotDataProvider* dataProvider READ dataProvider WRITE setDataProvider NOTIFY
                 dataProviderChanged)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

  Q_PROPERTY(QAbstractAxis* axisX READ axisX WRITE setAxisX NOTIFY axisXChanged)
  Q_PROPERTY(QAbstractAxis* axisY READ axisY WRITE setAxisY NOTIFY axisYChanged)

public:
  explicit AbstractPlotSeries(QObject* parent = nullptr);

  uint32_t id() const noexcept
  {
    return id_;
  }

  QString const& name() const noexcept
  {
    return name_;
  }

  AbstractPlotDataProvider* dataProvider() const
  {
    return provider_;
  }

  QAbstractAxis* axisX() noexcept
  {
    return axisX_;
  }
  QAbstractAxis* axisY() noexcept
  {
    return axisY_;
  }

signals:
  void dataProviderChanged();
  void nameChanged(QString const&);
  void transformsChanged();
  void axisXChanged(QAbstractAxis*);
  void axisYChanged(QAbstractAxis*);

public slots:
  void setName(QString const& name)
  {
    if(name != name_)
    {
      name_ = name;
      emit nameChanged(name_);
    }
  }

  void setDataProvider(AbstractPlotDataProvider* p)
  {
    if(provider_ != p)
    {
      provider_          = p;
      provider_->series_ = this;
      if(name_.isEmpty() and not provider_->name().isEmpty())
      {
        setName(provider_->name());
      }
      connect(provider_, &AbstractPlotDataProvider::dataChanged, this, [this] { setDirty(); });
      emit dataProviderChanged();
    }
  }

  void updateTransforms();

  void setAxisX(QAbstractAxis* xAxis);
  void setAxisY(QAbstractAxis* yAxis);

protected:
  PlotContext& context()
  {
    return ctx_;
  }
  void releaseResources() override;

  void* createMappedBuffer(qplot::TypeId type, size_t count, vk::BufferUsageFlagBits usage);

  auto initializeDataProvider()
  {
    return provider_->initializePlotContext(ctx_);
  }

  auto updateDataProvider()
  {
    return provider_->update(ctx_);
  }

  PlotContext ctx_;

private:
  static uint32_t s_instanceCount_;

  uint32_t id_;

  AbstractPlotDataProvider* provider_ = nullptr;

  QAbstractAxis* axisX_ = nullptr;
  QAbstractAxis* axisY_ = nullptr;

  QString name_;
};
