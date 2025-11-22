// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// Qt uses Dynamic Dispatcher...
// Either:
//  - use Qt's dispatcher (how to get it ???)
//  - include vulkan.hpp first !

#include <vulkansquirclerenderer.h>

#include <QMcu/Plot/VK/VulkanPipelineBuilder.hpp>

#include <QFile>

static const float vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};

const int UBUF_SIZE = 4;

void SquircleRenderer::draw()
{
  auto& vk = vkContext();
  auto& cb = vk.commandBuffer;

  const vk::DeviceSize ubufOffset = vk.currentFrameSlot * allocPerUbuf_;
  auto*                p          = vk.dev.mapMemory(ubufMem_, ubufOffset, allocPerUbuf_);
  float                t          = t_;
  memcpy(p, &t, 4);
  vk.dev.unmapMemory(ubufMem_);

  // Do not assume any state persists on the command buffer. (it may be a
  // brand new one that just started recording)

  cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);

  VkDeviceSize vbufOffset = 0;
  cb.bindVertexBuffers(0, 1, &vbuf_, &vbufOffset);

  const uint32_t dynamicOffset = allocPerUbuf_ * vk.currentFrameSlot;
  cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                        pipelineLayout_,
                        0,
                        1,
                        &ubufDescriptor_,
                        1,
                        &dynamicOffset);

  cb.draw(4, 1, 0, 0);
}

void SquircleRenderer::prepareShader(Stage stage)
{
  QString filename;
  if(stage == VertexStage)
  {
    filename = QLatin1String(":/scenegraph/vulkanunderqml/squircle.vert.spv");
  }
  else
  {
    Q_ASSERT(stage == FragmentStage);
    filename = QLatin1String(":/scenegraph/vulkanunderqml/squircle.frag.spv");
  }
  QFile f(filename);
  if(!f.open(QIODevice::ReadOnly))
    qFatal("Failed to read shader %s", qPrintable(filename));

  const QByteArray contents = f.readAll();

  if(stage == VertexStage)
  {
    ver_ = contents;
    Q_ASSERT(!ver_.isEmpty());
  }
  else
  {
    frag_ = contents;
    Q_ASSERT(!frag_.isEmpty());
  }
}

#define USE_BUILDER

bool SquircleRenderer::initialize()
{
  auto& vk = vkContext();

  Q_ASSERT(vk.framesInFlight <= 3);

  VkResult err;

#ifdef USE_BUILDER
  auto builder = VulkanPipelineBuilder{vk};
  builder.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleStrip);
  builder.addStage(":/scenegraph/vulkanunderqml/squircle.vert.spv",
                   vk::ShaderStageFlagBits::eVertex);
  builder.addStage(":/scenegraph/vulkanunderqml/squircle.frag.spv",
                   vk::ShaderStageFlagBits::eFragment);
#else
  if(ver_.isEmpty())
    prepareShader(VertexStage);
  if(frag_.isEmpty())
    prepareShader(FragmentStage);

  vk::BufferCreateInfo   bufferInfo{};
  vk::MemoryAllocateInfo allocInfo{};
  vk::MemoryRequirements memReq;
  uint32_t               memTypeIndex;
#endif

#ifdef USE_BUILDER
  size_t verticesBufferSize;
  std::tie(verticesBufferSize, vbuf_, vbufMem_) = vk.allocateBuffer(
      sizeof(vertices),
      vk::BufferUsageFlagBits::eVertexBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

#else
  bufferInfo.setSize(sizeof(vertices));
  bufferInfo.setUsage(vk::BufferUsageFlagBits::eVertexBuffer);
  vbuf_ = vk.dev.createBuffer(bufferInfo);

  memReq = vk.dev.getBufferMemoryRequirements(vbuf_);
  allocInfo.setAllocationSize(memReq.size);

  memTypeIndex = vk.findMemoryTypeIndex(memReq,
                                        vk::MemoryPropertyFlagBits::eHostVisible
                                            | vk::MemoryPropertyFlagBits::eHostCoherent);
  if(memTypeIndex == UINT32_MAX)
    qFatal("Failed to find host visible and coherent memory type");

  allocInfo.setMemoryTypeIndex(memTypeIndex);
  vbufMem_                  = vk.dev.allocateMemory(allocInfo);
  size_t verticesBufferSize = allocInfo.allocationSize;
#endif

  void* p = nullptr;
  p       = vk.dev.mapMemory(vbufMem_, 0, verticesBufferSize);
  if(!p)
    qFatal("Failed to map vertex buffer memory: %d", err);
  memcpy(p, vertices, sizeof(vertices));
  vk.dev.unmapMemory(vbufMem_);
  vk.dev.bindBufferMemory(vbuf_, vbufMem_, 0);

  // Now have a uniform buffer with enough space for the buffer data for each
  // (potentially) in-flight frame. (as we will write the contents every
  // frame, and so would need to wait for command buffer completion if there
  // was only one, and that would not be nice)

  // Could have three buffers and three descriptor sets, or one buffer and
  // one descriptor set and dynamic offset. We chose the latter in this
  // example.

  // We use one memory allocation for all uniform buffers, but then have to
  // watch out for the buffer offset alignment requirement, which may be as
  // large as 256 bytes.

#ifdef USE_BUILDER
  size_t ubufTotalSize;
  std::tie(allocPerUbuf_, ubufTotalSize, ubuf_, ubufMem_) = vk.allocateDynamicBuffer(
      UBUF_SIZE,
      vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
#else
  allocPerUbuf_ = aligned(UBUF_SIZE, vk.physDevProps.limits.minUniformBufferOffsetAlignment);

  bufferInfo.setSize(vk.framesInFlight * allocPerUbuf_);
  bufferInfo.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);
  ubuf_ = vk.dev.createBuffer(bufferInfo);

  memReq       = vk.dev.getBufferMemoryRequirements(ubuf_);
  memTypeIndex = vk.findMemoryTypeIndex(memReq,
                                        vk::MemoryPropertyFlagBits::eHostVisible
                                            | vk::MemoryPropertyFlagBits::eHostCoherent);
  if(memTypeIndex == UINT32_MAX)
    qFatal("Failed to find host visible and coherent memory type");

  allocInfo.setAllocationSize(memReq.size);
  allocInfo.setMemoryTypeIndex(memTypeIndex);
  ubufMem_ = vk.dev.allocateMemory(allocInfo);
#endif

  vk.dev.bindBufferMemory(ubuf_, ubufMem_, 0);

  // Now onto the pipeline.

  vk::DescriptorSetLayoutBinding descLayoutBinding{};
  descLayoutBinding.setBinding(0);
  descLayoutBinding.setDescriptorCount(1);
  descLayoutBinding.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
  descLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex
                                  | vk::ShaderStageFlagBits::eFragment);

#ifdef USE_BUILDER
  builder.descSetLayoutBindings.push_back(descLayoutBinding);

  builder.vertexBinding.setStride(2 * sizeof(float));
  builder.vertexAttr.setFormat(vk::Format::eR32G32Sfloat);

  vk::Result res;

  auto const& [pipeline, cache, layout, setLayouts] = builder.build();
  pipeline_                                         = pipeline;
  pipelineCache_                                    = cache;
  pipelineLayout_                                   = layout;
  resLayout_                                        = setLayouts.at(0);
#else

  pipelineCache_ = vk.dev.createPipelineCache(vk::PipelineCacheCreateInfo{});

  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  vk::GraphicsPipelineCreateInfo    pipelineInfo{};
  resLayout_ = vk.dev.createDescriptorSetLayout(layoutInfo);

  layoutInfo.setBindingCount(1);
  layoutInfo.setPBindings(&descLayoutBinding);
  resLayout_ = vk.dev.createDescriptorSetLayout(layoutInfo);

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.setSetLayoutCount(1);
  pipelineLayoutInfo.setPSetLayouts(&resLayout_);
  pipelineLayout_ = vk.dev.createPipelineLayout(pipelineLayoutInfo);

  vk::ShaderModuleCreateInfo shaderInfo;
  shaderInfo.setCodeSize(ver_.size());
  shaderInfo.setPCode(reinterpret_cast<const quint32*>(ver_.constData()));
  vk::ShaderModule vertShaderModule = vk.dev.createShaderModule(shaderInfo);

  shaderInfo.setCodeSize(frag_.size());
  shaderInfo.setPCode(reinterpret_cast<const quint32*>(frag_.constData()));
  vk::ShaderModule fragShaderModule = vk.dev.createShaderModule(shaderInfo);

  vk::PipelineShaderStageCreateInfo stageInfo[2]{};
  stageInfo[0].setStage(vk::ShaderStageFlagBits::eVertex);
  stageInfo[0].setModule(vertShaderModule);
  stageInfo[0].setPName("main");
  stageInfo[1].setStage(vk::ShaderStageFlagBits::eFragment);
  stageInfo[1].setModule(fragShaderModule);
  stageInfo[1].setPName("main");
  pipelineInfo.setStages(stageInfo);

  vk::VertexInputBindingDescription   vertexBinding{0,
                                                  2 * sizeof(float),
                                                  vk::VertexInputRate::eVertex};
  vk::VertexInputAttributeDescription vertexAttr = {
      0,                         // location
      0,                         // binding
      vk::Format::eR32G32Sfloat, // VK_FORMAT_R32G32_SFLOAT, // 'vertices' only has 2 floats per
                                 // vertex
      0                          // offset
  };
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.setVertexBindingDescriptionCount(1);
  vertexInputInfo.setPVertexBindingDescriptions(&vertexBinding);
  vertexInputInfo.setVertexAttributeDescriptionCount(1);
  vertexInputInfo.setPVertexAttributeDescriptions(&vertexAttr);
  pipelineInfo.setPVertexInputState(&vertexInputInfo);

  vk::DynamicState dynStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicInfo;
  dynamicInfo.setDynamicStates(dynStates);
  pipelineInfo.setPDynamicState(&dynamicInfo);

  vk::PipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.viewportCount = viewportInfo.scissorCount = 1;
  pipelineInfo.pViewportState                            = &viewportInfo;

  vk::PipelineInputAssemblyStateCreateInfo iaInfo;
  iaInfo.topology                  = vk::PrimitiveTopology::eTriangleStrip;
  pipelineInfo.pInputAssemblyState = &iaInfo;

  vk::PipelineRasterizationStateCreateInfo rsInfo{};
  rsInfo.lineWidth                 = 1.0f;
  pipelineInfo.pRasterizationState = &rsInfo;

  vk::PipelineMultisampleStateCreateInfo msInfo{};
  msInfo.rasterizationSamples    = vk.rasterizationSamples;
  pipelineInfo.pMultisampleState = &msInfo;

  vk::PipelineDepthStencilStateCreateInfo dsInfo{};
  pipelineInfo.pDepthStencilState = &dsInfo;
  // SrcAlpha, One
  vk::PipelineColorBlendStateCreateInfo blendInfo{};
  vk::PipelineColorBlendAttachmentState blend;
  blend.blendEnable         = true;
  blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha; // VK_BLEND_FACTOR_SRC_ALPHA;
  blend.dstColorBlendFactor = vk::BlendFactor::eOne;      // VK_BLEND_FACTOR_ONE;
  blend.colorBlendOp        = vk::BlendOp::eAdd;          // VK_BLEND_OP_ADD;
  blend.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha; // VK_BLEND_FACTOR_SRC_ALPHA;
  blend.dstAlphaBlendFactor = vk::BlendFactor::eOne;      // VK_BLEND_FACTOR_ONE;
  blend.alphaBlendOp        = vk::BlendOp::eAdd;          // VK_BLEND_OP_ADD;
  blend.colorWriteMask      = vk::ColorComponentFlagBits::eR
                       | vk::ColorComponentFlagBits::eG
                       | vk::ColorComponentFlagBits::eB
                       | vk::ColorComponentFlagBits::eA;
  blendInfo.attachmentCount     = 1;
  blendInfo.pAttachments        = &blend;
  pipelineInfo.pColorBlendState = &blendInfo;

  pipelineInfo.layout = pipelineLayout_;

  pipelineInfo.renderPass = vk.rp;

  vk::Result res;
  std::tie(res, pipeline_) = vk.dev.createGraphicsPipeline(pipelineCache_, pipelineInfo);

  vk.dev.destroyShaderModule(vertShaderModule);
  vk.dev.destroyShaderModule(fragShaderModule);

  if(res != vk::Result::eSuccess)
  {
    qFatal("Failed to create graphics pipeline: %d", err);
  }
#endif

  // Now just need some descriptors.
  vk::DescriptorPoolSize descPoolSizes[] = {
      {vk::DescriptorType::eUniformBufferDynamic, 1}
  };
  vk::DescriptorPoolCreateInfo descPoolInfo{};
  descPoolInfo.maxSets = 1;
  descPoolInfo.setPoolSizes(descPoolSizes);
  descriptorPool_ = vk.dev.createDescriptorPool(descPoolInfo);

  vk::DescriptorSetAllocateInfo descAllocInfo{};
  descAllocInfo.descriptorPool     = descriptorPool_;
  descAllocInfo.descriptorSetCount = 1;
  descAllocInfo.pSetLayouts        = &resLayout_;
  ubufDescriptor_                  = vk.dev.allocateDescriptorSets(descAllocInfo)[0];

  vk::WriteDescriptorSet writeInfo{};
  writeInfo.dstSet          = ubufDescriptor_;
  writeInfo.dstBinding      = 0;
  writeInfo.descriptorCount = 1;
  writeInfo.descriptorType  = vk::DescriptorType::eUniformBufferDynamic;

  vk::DescriptorBufferInfo bufInfo{};
  bufInfo.buffer        = ubuf_;
  bufInfo.offset        = 0; // dynamic offset is used so this is ignored
  bufInfo.range         = UBUF_SIZE;
  writeInfo.pBufferInfo = &bufInfo;
  vk.dev.updateDescriptorSets(1, &writeInfo, 0, nullptr);

  return true;
}

void SquircleRenderer::releaseResources()
{
  qDebug("cleanup");
  if(!hasContext())
  {
    return;
  }

  auto& vk = vkContext();
  if(!vk.dev)
  {
    return;
  }

  vk.dev.destroy(pipeline_);
  vk.dev.destroy(pipelineLayout_);
  vk.dev.destroy(resLayout_);
  vk.dev.destroy(descriptorPool_);
  vk.dev.destroy(pipelineCache_);
  vk.dev.destroy(vbuf_);
  vk.dev.free(vbufMem_);
  vk.dev.destroy(ubuf_);
  vk.dev.free(ubufMem_);

  qDebug("released");
}