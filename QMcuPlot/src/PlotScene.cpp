#include <QMcu/Plot/PlotScene.hpp>

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QVulkanInstance>

#include <QMcu/Plot/Plot.hpp>

#include <rhi/qrhi.h>

#include <magic_enum/magic_enum.hpp>

VkDebugUtilsMessengerEXT sMessenger = nullptr;

VKAPI_ATTR VkBool32 VKAPI_CALL
    genericDebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                         vk::DebugUtilsMessageTypeFlagsEXT             messageTypes,
                         const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                         void*                                         pUserData)
{
  if(messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
  {
    qCritical().nospace()
        << pCallbackData->messageIdNumber
        << "Validation Layer: Error: "
        << pCallbackData->pMessageIdName
        << ": "
        << pCallbackData->pMessage;
  }
  else if(messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
  {
    qWarning().nospace()
        << pCallbackData->messageIdNumber
        << "Validation Layer: Warning: "
        << pCallbackData->pMessageIdName
        << ": "
        << pCallbackData->pMessage;
  }
  else if(messageTypes & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
  {
    qInfo().nospace()
        << pCallbackData->messageIdNumber
        << "Validation Layer: Performance: "
        << pCallbackData->pMessageIdName
        << ": "
        << pCallbackData->pMessage;
  }
  else
  {
    qInfo().nospace()
        << pCallbackData->messageIdNumber
        << "Validation Layer: Info: "
        << pCallbackData->pMessageIdName
        << ": "
        << pCallbackData->pMessage;
  }
  return VK_FALSE;
}

QElapsedTimer PlotScene::sTimer_ = []
{
  QElapsedTimer t;
  t.start();
  return t;
}();

PlotScene::PlotScene(QQuickWindow* win) : QSGRenderNode(), win_(win)
{
}

PlotScene::~PlotScene()
{
  releaseResources();

  for(auto* r : renderers_)
  {
    if(r->parent() == nullptr)
    {
      delete r;
    }
  }
}

std::vector<glm::vec2>
    makeRoundedRectVertices(float width, float height, float radius, int segmentsPerCorner)
{
  std::vector<glm::vec2> verts;

  auto corner = [&](float cx, float cy, float startAngle)
  {
    for(int i = 0; i <= segmentsPerCorner; ++i)
    {
      float angle = startAngle + i * (M_PI_2 / segmentsPerCorner);
      verts.push_back({glm::vec2(cx + cos(angle) * radius, cy + sin(angle) * radius)});
    }
  };

  // Triangle fan: center first
  verts.push_back({glm::vec2(0.0f, 0.0f)});

  // Top-right corner (0)
  corner(width / 2 - radius, height / 2 - radius, 0.0f);
  // Top-left corner (1)
  corner(-width / 2 + radius, height / 2 - radius, M_PI_2);
  // Bottom-left (2)
  corner(-width / 2 + radius, -height / 2 + radius, M_PI);
  // Bottom-right (3)
  corner(width / 2 - radius, -height / 2 + radius, 3 * M_PI_2);

  // close fan by repeating first perimeter vertex
  verts.push_back(verts[1]);

  return verts;
}

#define ENABLE_STENCIL_MASK

void PlotScene::setupStencilPipeline()
{
#ifdef ENABLE_STENCIL_MASK
  auto& vk  = vk_;
  auto& dev = vk.dev;

  stencilPipelineCache_ = vk.dev.createPipelineCache(vk::PipelineCacheCreateInfo{});

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};

  vk::PushConstantRange pushRange{};
  pushRange.stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eGeometry;
  pushRange.offset     = 0;
  pushRange.size       = sizeof(stencilUbo);
  pipelineLayoutInfo.setPushConstantRanges(pushRange);

  stencilPipelineLayout_ = vk.dev.createPipelineLayout(pipelineLayoutInfo);

  auto vertShaderModule = vk.createShaderModule("stencil.vert.spv");
  auto geomShaderModule = vk.createShaderModule("stencil.geom.spv");
  auto fragShaderModule = vk.createShaderModule("stencil.frag.spv");

  vk::PipelineShaderStageCreateInfo stageInfo[3]{};
  stageInfo[0].setStage(vk::ShaderStageFlagBits::eVertex);
  stageInfo[0].setModule(vertShaderModule);
  stageInfo[0].setPName("main");
  stageInfo[1].setStage(vk::ShaderStageFlagBits::eGeometry);
  stageInfo[1].setModule(geomShaderModule);
  stageInfo[1].setPName("main");
  stageInfo[2].setStage(vk::ShaderStageFlagBits::eFragment);
  stageInfo[2].setModule(fragShaderModule);
  stageInfo[2].setPName("main");

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{}; // dummy - no vertex input
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

  vk::StencilOpState stencilOp{};
  stencilOp.failOp      = vk::StencilOp::eKeep;
  stencilOp.passOp      = vk::StencilOp::eReplace; // write 1 where shape exists
  stencilOp.depthFailOp = vk::StencilOp::eKeep;
  stencilOp.compareOp   = vk::CompareOp::eAlways;
  stencilOp.reference   = 1;
  stencilOp.compareMask = 0xFF;
  stencilOp.writeMask   = 0xFF;

  vk::PipelineDepthStencilStateCreateInfo dsInfo{};
  dsInfo.stencilTestEnable = true;
  dsInfo.back = dsInfo.front = stencilOp;

  // SrcAlpha, One
  vk::PipelineColorBlendStateCreateInfo blendInfo{};
  vk::PipelineColorBlendAttachmentState blend{};
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
  pipelineInfo.layout     = stencilPipelineLayout_;
  pipelineInfo.renderPass = vk.rp;

  vk::Result res;
  std::tie(res, stencilPipeline_) =
      vk.dev.createGraphicsPipeline(stencilPipelineCache_, pipelineInfo);

  vk.dev.destroyShaderModule(vertShaderModule);
  vk.dev.destroyShaderModule(fragShaderModule);
  vk.dev.destroyShaderModule(geomShaderModule);

  if(res != vk::Result::eSuccess)
  {
    qFatal().nospace() << "Failed to create graphics pipeline: " << magic_enum::enum_name(res);
  }

  // apply to VulkanContext

  vk.sceneDS.depthTestEnable   = false;
  vk.sceneDS.stencilTestEnable = true;

  vk.stencilTest.failOp      = vk::StencilOp::eKeep;
  vk.stencilTest.passOp      = vk::StencilOp::eKeep;
  vk.stencilTest.depthFailOp = vk::StencilOp::eKeep;
  vk.stencilTest.compareOp   = vk::CompareOp::eEqual; // only pass where stencil==1
  vk.stencilTest.compareMask = 0xFF;
  vk.stencilTest.writeMask   = 0x00; // do not modify stencil
  vk.stencilTest.reference   = 1;

  vk.sceneDS.front = vk.sceneDS.back = vk.stencilTest;
#endif
}

void PlotScene::prepare()
{
  if(not initialized_)
  {
    // We are not prepared for anything other than running with the RHI and its Vulkan backend.
    QSGRendererInterface* rif = win_->rendererInterface();
    Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::Vulkan);

    QVulkanInstance* inst = reinterpret_cast<QVulkanInstance*>(
        rif->getResource(win_, QSGRendererInterface::VulkanInstanceResource));
    Q_ASSERT(inst && inst->isValid());

    auto& vk = vk_;

    vk.phyDev = *reinterpret_cast<VkPhysicalDevice*>(
        rif->getResource(win_, QSGRendererInterface::PhysicalDeviceResource));

    vk.dev =
        *reinterpret_cast<VkDevice*>(rif->getResource(win_, QSGRendererInterface::DeviceResource));
    Q_ASSERT(vk.phyDev && vk.dev);

    vk.rp = *reinterpret_cast<VkRenderPass*>(
        rif->getResource(win_, QSGRendererInterface::RenderPassResource));
    Q_ASSERT(vk.rp);

    vk.physDevProps    = vk.phyDev.getProperties();
    vk.physDevMemProps = vk.phyDev.getMemoryProperties();
    vk.framesInFlight  = win_->graphicsStateInfo().framesInFlight;

    setupStencilPipeline();

    initialized_ = true;
  }
}

void PlotScene::render(const RenderState* state)
{
  QSGRendererInterface* rif = win_->rendererInterface();

  for(auto* r : renderers_)
  {
    if(not r->isInitialized())
    {
      r->vk_          = &vk_;
      r->initialized_ = r->initialize();
    }
  }

  auto& vk = vk_;

  // This example demonstrates the simple case: prepending some commands to
  // the scenegraph's main renderpass. It does not create its own passes,
  // rendertargets, etc. so no synchronization is needed.

  const QQuickWindow::GraphicsStateInfo& stateInfo(win_->graphicsStateInfo());

  vk.currentFrameSlot    = stateInfo.currentFrameSlot;
  vk.modelViewProjection = toGlm(*projectionMatrix() * *matrix());

  win_->beginExternalCommands();

  // Must query the command buffer _after_ beginExternalCommands(), this is
  // actually important when running on Vulkan because what we get here is a
  // new secondary command buffer, not the primary one.
  auto& cb = vk.commandBuffer = *reinterpret_cast<VkCommandBuffer*>(
      rif->getResource(win_, QSGRendererInterface::CommandListResource));
  Q_ASSERT(vk.commandBuffer);

  const QSize renderTargetSize = renderTarget()->pixelSize();
  vk.viewPort.setX(0);
  vk.viewPort.setY(0);
  vk.viewPort.setWidth(renderTargetSize.width());
  vk.viewPort.setHeight(renderTargetSize.height());

  vk.scissor.setOffset({0, 0});
  vk.scissor.setExtent({uint32_t(renderTargetSize.width()), uint32_t(renderTargetSize.height())});

  cb.setViewport(0, 1, &vk.viewPort);
  cb.setScissor(0, 1, &vk.scissor);

#ifdef ENABLE_STENCIL_MASK
  { // compute stencil mask
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, stencilPipeline_);

    stencilUbo.mvp            = vk.modelViewProjection;
    stencilUbo.boundingSize.x = vk.boundingRect.extent.width;
    stencilUbo.boundingSize.y = vk.boundingRect.extent.height;
    cb.pushConstants(stencilPipelineLayout_,
                     vk::ShaderStageFlagBits::eFragment
                         | vk::ShaderStageFlagBits::eGeometry
                         | vk::ShaderStageFlagBits::eFragment,
                     0,
                     sizeof(stencilUbo),
                     &stencilUbo);
    cb.draw(4, 1, 0, 0);
  }
#endif

  for(auto* r : renderers_)
  {
    if(r->isInitialized())
    {
      r->draw();
    }
  }

  win_->endExternalCommands();
}

void PlotScene::releaseResources()
{
  for(auto* r : renderers_)
  {
    if(r->isInitialized())
    {
      r->releaseResources();
      r->initialized_ = false;
    }
  }

#ifdef ENABLE_STENCIL_MASK
  auto& vk = vk_;
  if(stencilPipeline_)
  {
    vk.dev.destroy(stencilPipeline_);
    vk.dev.destroy(stencilPipelineCache_);
    vk.dev.destroy(stencilPipelineLayout_);
  }
#endif
}

QSGRenderNode::RenderingFlags PlotScene::flags() const
{
  return RenderingFlag::BoundedRectRendering | RenderingFlag::DepthAwareRendering;
}

QSGRenderNode::StateFlags PlotScene::changedStates() const
{
  return StateFlag::DepthState
       | StateFlag::StencilState
       | StateFlag::ScissorState
       | StateFlag::ColorState
       | StateFlag::BlendState
       | StateFlag::CullState
       | StateFlag::ViewportState;
}
