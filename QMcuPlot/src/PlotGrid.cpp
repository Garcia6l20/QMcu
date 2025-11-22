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
  auto& vk = vkContext();

  auto builder = VulkanPipelineBuilder(vk);
  builder.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eLineList);

  builder.addStage("grid.vert.spv", vk::ShaderStageFlagBits::eVertex);
  builder.addStage("grid.frag.spv", vk::ShaderStageFlagBits::eFragment);

  const size_t verticesCount = ticks_ * 2 * 2; // 2 per ticks, vertical + horizontal
  size_t       verticesBufferSize;
  std::tie(verticesBufferSize, vbuf_, vbufMem_) = builder.allocateBuffer(
      2 * sizeof(float) * verticesCount,
      vk::BufferUsageFlagBits::eVertexBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

  auto p = reinterpret_cast<float*>(vk.dev.mapMemory(vbufMem_, 0, verticesBufferSize));
  if(!p)
    qFatal("Failed to map vertex buffer memory");
  for(int ii = 0; ii < ticks_; ++ii)
  {
    const float r = (ii + 1) / float(ticks_ + 1);

    // Vertical line
    p[8 * ii]     = r;
    p[8 * ii + 1] = -1.0;
    p[8 * ii + 2] = r;
    p[8 * ii + 3] = 1.0;

    // Horizontal line
    p[8 * ii + 4] = -1.0;
    p[8 * ii + 5] = r;
    p[8 * ii + 6] = 1.0;
    p[8 * ii + 7] = r;
  }
  vk.dev.unmapMemory(vbufMem_);
  vk.dev.bindBufferMemory(vbuf_, vbufMem_, 0);

  builder.vertexBinding.setStride(2 * sizeof(float));
  builder.vertexAttr.setFormat(vk::Format::eR32G32Sfloat);

  vk::PipelineRasterizationLineStateCreateInfo lineInfo{};
  lineInfo.lineRasterizationMode = vk::LineRasterizationModeEXT::eRectangularKHR;
  lineInfo.stippledLineEnable    = true;
  lineInfo.lineStippleFactor     = 1;
  lineInfo.lineStipplePattern    = 0b1100001111000011;
  builder.rasterizationInfo.setPNext(&lineInfo);
  builder.rasterizationInfo.setLineWidth(1.0f);

  builder.pushConstantsRange.setStageFlags(vk::ShaderStageFlagBits::eVertex
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

    dev.destroy(vbuf_);
    dev.free(vbufMem_);
  }
}

void PlotGrid::draw()
{
  auto& vk = vkContext();
  auto& cb = vk.commandBuffer;

  cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);

  VkDeviceSize vbufOffset = 0;
  cb.bindVertexBuffers(0, 1, &vbuf_, &vbufOffset);

  push_.mvp            = vk.modelViewProjection;
  push_.boundingSize.x = vk.boundingRect.extent.width;
  push_.boundingSize.y = vk.boundingRect.extent.height;
  cb.pushConstants(pipelineLayout_,
                   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                   0,
                   sizeof(push_),
                   &push_);

  cb.draw(ticks_ * 2 * 2, 1, 0, 0);
}
