#ifndef VULKANSQUIRCLERENDERER_H
#define VULKANSQUIRCLERENDERER_H

#include <QMcu/Plot/PlotSceneItem.hpp>

class SquircleRenderer : public PlotSceneItem
{
  Q_OBJECT
public:
  SquircleRenderer() = default;
  ~SquircleRenderer() = default;

  void setT(qreal t)
  {
    t_ = t;
  }

protected:
  bool doInitialize() final;
  void doDraw() final;
  void doReleaseResources() final;

private:
  enum Stage
  {
    VertexStage,
    FragmentStage
  };
  void prepareShader(Stage stage);

  qreal t_ = 0;

  QByteArray ver_;
  QByteArray frag_;

  vk::Buffer       vbuf_{};
  vk::DeviceMemory vbufMem_{};
  vk::Buffer       ubuf_{};
  vk::DeviceMemory ubufMem_{};
  vk::DeviceSize   allocPerUbuf_ = 0;

  vk::PipelineLayout      pipelineLayout_{};
  vk::DescriptorSetLayout resLayout_{};
  vk::Pipeline            pipeline_{};

  vk::DescriptorPool descriptorPool_{};
  vk::DescriptorSet  ubufDescriptor_{};
};

#endif
