#pragma once

#include <QMcu/Plot/PlotSceneItem.hpp>

#include <QColor>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

class PlotGrid : public PlotSceneItem
{
  Q_OBJECT

  Q_PROPERTY(uint32_t ticks READ ticks WRITE setTicks NOTIFY ticksChanged FINAL)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
  explicit PlotGrid(QObject* parent = nullptr);

  uint32_t ticks() const noexcept
  {
    return ticks_;
  }

  QColor const& color() const noexcept
  {
    return color_;
  }

public slots:
  void setTicks(uint32_t ticks)
  {
    if(ticks != ticks_)
    {
      if(isInitialized())
      {
        qWarning() << "Cannot change tick count after beeing initialized";
        return;
      }
      ticks_ = ticks;
      setDirty();
      emit ticksChanged(ticks_);
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
  bool doInitialize() final;
  void doDraw() final;
  void doReleaseResources() final;

private:
  QColor color_ = Qt::GlobalColor::lightGray;

  struct GridPush
  {
    glm::mat4 mvp;
    glm::vec4 color{0.5f, 0.5f, 0.5f, 0.7f};
    glm::vec2 boundingSize;
  } push_;

  uint32_t ticks_ = 5;

  vk::Buffer       vbuf_{};
  vk::DeviceMemory vbufMem_{};
};
