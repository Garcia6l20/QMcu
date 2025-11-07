#include <QMcu/Plot/PlotGrid.hpp>

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

  Q_ASSERT(vk.framesInFlight <= 3);

  pipelineCache_ = vk.dev.createPipelineCache(vk::PipelineCacheCreateInfo{});

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayout_ = vk.dev.createPipelineLayout(pipelineLayoutInfo);

  auto vertShaderModule = vk.createShaderModule("grid.vert.spv");
  auto geomShaderModule = vk.createShaderModule("grid.geom.spv");
  auto fragShaderModule = vk.createShaderModule("grid.frag.spv");

  vk::PipelineShaderStageCreateInfo stageInfo[3]{};
  memset(&stageInfo, 0, sizeof(stageInfo));
  stageInfo[0].setStage(vk::ShaderStageFlagBits::eVertex);
  stageInfo[0].setModule(vertShaderModule);
  stageInfo[0].setPName("main");
  stageInfo[1].setStage(vk::ShaderStageFlagBits::eGeometry);
  stageInfo[1].setModule(geomShaderModule);
  stageInfo[1].setPName("main");
  stageInfo[2].setStage(vk::ShaderStageFlagBits::eFragment);
  stageInfo[2].setModule(fragShaderModule);
  stageInfo[2].setPName("main");

  vk::VertexInputBindingDescription   vertexBinding{0,
                                                  2 * sizeof(float),
                                                  vk::VertexInputRate::eVertex};
  vk::VertexInputAttributeDescription vertexAttr = {
      0,                         // location
      0,                         // binding
      vk::Format::eR32G32Sfloat, // 'vertices' only has 2 floats per vertex
      0                          // offset
  };
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.setVertexBindingDescriptionCount(1);
  vertexInputInfo.setPVertexBindingDescriptions(&vertexBinding);
  vertexInputInfo.setVertexAttributeDescriptionCount(1);
  vertexInputInfo.setPVertexAttributeDescriptions(&vertexAttr);

  vk::DynamicState dynStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicInfo;
  dynamicInfo.setDynamicStates(dynStates);

  vk::PipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.viewportCount = viewportInfo.scissorCount = 1;

  vk::PipelineInputAssemblyStateCreateInfo iaInfo;
  iaInfo.topology = vk::PrimitiveTopology::ePointList;

  vk::PipelineRasterizationStateCreateInfo rsInfo{};
  rsInfo.lineWidth = 1.0f;

  vk::PipelineMultisampleStateCreateInfo msInfo{};
  msInfo.rasterizationSamples = vk.rasterizationSamples;

  vk::PipelineDepthStencilStateCreateInfo dsInfo = vk.sceneDS;

  // SrcAlpha, One
  vk::PipelineColorBlendStateCreateInfo blendInfo{};
  vk::PipelineColorBlendAttachmentState blend;
  blend.blendEnable         = true;
  blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
  blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
  blend.colorBlendOp        = vk::BlendOp::eAdd;
  blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
  blend.dstAlphaBlendFactor = vk::BlendFactor::eZero;
  blend.alphaBlendOp        = vk::BlendOp::eAdd;
  blend.colorWriteMask      = vk::ColorComponentFlagBits::eR
                       | vk::ColorComponentFlagBits::eG
                       | vk::ColorComponentFlagBits::eB
                       | vk::ColorComponentFlagBits::eA;
  blendInfo.attachmentCount = 1;
  blendInfo.pAttachments    = &blend;

  vk::GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.pViewportState      = &viewportInfo;
  pipelineInfo.pInputAssemblyState = &iaInfo;
  pipelineInfo.pRasterizationState = &rsInfo;
  pipelineInfo.pMultisampleState   = &msInfo;
  pipelineInfo.pDepthStencilState  = &dsInfo;
  pipelineInfo.pColorBlendState    = &blendInfo;
  pipelineInfo.setStages(stageInfo);
  pipelineInfo.setPVertexInputState(&vertexInputInfo);
  pipelineInfo.setPDynamicState(&dynamicInfo);
  pipelineInfo.layout     = pipelineLayout_;
  pipelineInfo.renderPass = vk.rp;

  vk::Result res;
  std::tie(res, pipeline_) = vk.dev.createGraphicsPipeline(pipelineCache_, pipelineInfo);

  vk.dev.destroyShaderModule(vertShaderModule);
  vk.dev.destroyShaderModule(fragShaderModule);

  if(res != vk::Result::eSuccess)
  {
    qFatal().nospace() << "Failed to create graphics pipeline: " << magic_enum::enum_name(res);
  }

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
