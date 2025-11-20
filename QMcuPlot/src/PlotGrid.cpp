#include <QMcu/Plot/PlotGrid.hpp>
#include <QMcu/Plot/VK/VulkanPipelineBuilder.hpp>

#include <Logging.hpp>

#include <QColor>
#include <QFile>

#include <glm/vec4.hpp>

#include <ranges>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <magic_enum/magic_enum.hpp>

PlotGrid::PlotGrid(QObject* parent) : PlotSceneItem{parent}
{
}

QByteArray getShader(QString const& filename)
{
  QFile f(QString(":/qmcu/plot/shaders/%1").arg(filename));
  if(!f.open(QIODevice::ReadOnly))
  {
    qFatal("Failed to read shader %s", qPrintable(filename));
  }

  return f.readAll();
}

bool PlotGrid::initialize()
{
  auto builder = VulkanPipelineBuilder(vkContext());
  builder.addStage("grid.vert.spv", vk::ShaderStageFlagBits::eVertex);
  builder.addStage("grid.geom.spv", vk::ShaderStageFlagBits::eGeometry);
  builder.addStage("grid.frag.spv", vk::ShaderStageFlagBits::eFragment);

  builder.pushConstantsRange.setStageFlags(vk::ShaderStageFlagBits::eVertex
                                           | vk::ShaderStageFlagBits::eGeometry
                                           | vk::ShaderStageFlagBits::eFragment);
  builder.pushConstantsRange.setSize(sizeof(push_));

  std::vector<vk::DescriptorSetLayout> dsl;
  std::tie(pipeline_, pipelineCache_, pipelineLayout_, dsl) = builder.build();

  return true;
}

void PlotGrid::releaseResources()
{
  if(pipeline_ != nullptr)
  {
    auto& vk  = vkContext();
    auto& dev = vk.dev;
    dev.destroy(pipeline_);
    dev.destroy(pipelineLayout_);
    dev.destroy(pipelineCache_);
  }
}

void PlotGrid::draw()
{
  auto& vk = vkContext();
  auto& cb = vk.commandBuffer;

  cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);

  push_.mvp            = vk.modelViewProjection;
  push_.boundingSize.x = vk.boundingRect.extent.width;
  push_.boundingSize.y = vk.boundingRect.extent.height;
  cb.pushConstants(pipelineLayout_,
                   vk::ShaderStageFlagBits::eVertex
                       | vk::ShaderStageFlagBits::eGeometry
                       | vk::ShaderStageFlagBits::eFragment,
                   0,
                   sizeof(push_),
                   &push_);

  cb.draw(push_.ticks, 1, 0, 0);
}
