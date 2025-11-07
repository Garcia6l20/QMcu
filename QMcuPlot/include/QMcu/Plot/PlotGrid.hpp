#pragma once

#include <QMcu/Plot/PlotSceneItem.hpp>

#include <QColor>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

class PlotGrid : public PlotSceneItem
{
  Q_OBJECT

  Q_PROPERTY(uint32_t ticks READ ticks WRITE setTicks NOTIFY ticksChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
  explicit PlotGrid(QObject* parent = nullptr);

  uint32_t ticks() const noexcept
  {
    return push_.ticks;
  }

  QColor const& color() const noexcept
  {
    return color_;
  }

public slots:
  void setTicks(uint32_t ticks)
  {
    if(ticks != push_.ticks)
    {
      push_.ticks = ticks;
      setDirty();
      emit ticksChanged(push_.ticks);
    }
  }

  void setColor(QColor const& color) noexcept
  {
    if(color != color_)
    {
      color_      = color;
      push_.color = glm::vec4{color_.redF(), color_.greenF(), color_.blueF(), color_.alphaF()};
      setDirty();
      emit colorChanged(color_);
    }
  }

signals:
  void ticksChanged(uint32_t);
  void colorChanged(QColor const&);

protected:
  bool initialize() final;
  void draw() final;
  void releaseResources() final;

private:
  QColor color_ = Qt::GlobalColor::lightGray;

  struct GridPush
  {
    glm::mat4 mvp;
    glm::vec4 color{0.5f, 0.5f, 0.5f, 0.7f};
    glm::vec2 boundingSize;
    uint32_t  ticks = 5;
    uint32_t  dash  = 12;
    uint32_t  gap   = 4;
  } push_;

  vk::Pipeline       pipeline_;
  vk::PipelineCache  pipelineCache_;
  vk::PipelineLayout pipelineLayout_;
};
