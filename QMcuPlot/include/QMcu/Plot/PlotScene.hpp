#pragma once

#include <QMcu/Plot/AbstractPlotSeries.hpp>
#include <QMcu/Plot/PlotContext.hpp>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLFunctions_4_5_Core>

#include <QQuickWindow>
#include <QSGRenderNode>

#include <QMcu/Plot/VK/VulkanContext.hpp>

class PlotSceneItem;

class PlotScene : public QSGRenderNode
{
public:
  PlotScene(QQuickWindow* win);
  virtual ~PlotScene() = default;

  void setBoundingRect(const QRectF& boundingRect)
  {
    if(boundingRect_ != boundingRect)
    {
      boundingRect_    = boundingRect;
      vk_.boundingRect = vk::Rect2D{
          {int32_t(boundingRect_.x()),      int32_t(boundingRect_.y())      },
          {uint32_t(boundingRect_.width()), uint32_t(boundingRect_.height())}
      };
    }
  }

  void setBorder(float border) noexcept
  {
    stencilUbo.border = border;
  }

  void setRadius(float radius) noexcept
  {
    stencilUbo.radius = radius;
  }

  void addRenderer(PlotSceneItem* renderer)
  {
    if(not renderers_.contains(renderer))
    {
      renderers_.append(renderer);
    }
  }

  void removeRenderer(PlotSceneItem* renderer)
  {
    renderers_.removeAll(renderer);
  }

  void                      prepare() final;
  void                      render(const RenderState* state) final;
  void                      releaseResources() final;
  RenderingFlags            flags() const final;
  QSGRenderNode::StateFlags changedStates() const final;

private:
  QQuickWindow*         win_ = nullptr;
  QList<PlotSceneItem*> renderers_;
  VulkanContext         vk_;
  QRectF                boundingRect_;

  bool initialized_ = false;

  void setupStencilPipeline();

  struct StencilUBO
  {
    glm::mat4 mvp;
    glm::vec4 color{1.0, 1.0, 1.0, 1.0};
    glm::vec2 boundingSize;
    float     border;
    float     radius;
  } stencilUbo;

  vk::PipelineCache  stencilPipelineCache_;
  vk::PipelineLayout stencilPipelineLayout_;
  vk::Pipeline       stencilPipeline_;

  static QElapsedTimer sTimer_;
};
