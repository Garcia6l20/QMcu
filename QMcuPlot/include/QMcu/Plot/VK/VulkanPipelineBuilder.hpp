#pragma once

#include <QMcu/Plot/VK/VulkanContext.hpp>

class VulkanPipelineBuilder
{
public:
  VulkanPipelineBuilder(VulkanContext& ctx) : vk{ctx}
  {
  }

  ~VulkanPipelineBuilder()
  {
    for(auto& module : shaderModules)
    {
      vk.dev.destroy(module);
    }
  }

  auto& addStage(QString const&          shaderResource,
                 vk::ShaderStageFlagBits stage,
                 std::string_view        name = "main")
  {
    auto& module = shaderModules.emplace_back(vk.createShaderModule(shaderResource));
    return stageInfos.emplace_back(vk::PipelineShaderStageCreateFlags{},
                                   stage,
                                   module,
                                   name.data());
  }

  auto allocateBuffer(size_t size, vk::BufferUsageFlagBits usage, vk::MemoryPropertyFlags memProps)
  {
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.setSize(size);
    bufferInfo.setUsage(usage);

    auto buf          = vk.dev.createBuffer(bufferInfo);
    auto memReq       = vk.dev.getBufferMemoryRequirements(buf);
    auto memTypeIndex = vk.findMemoryTypeIndex(memReq, memProps);
    if(memTypeIndex == UINT32_MAX)
      qFatal("Failed to find host visible and coherent memory type");

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.setAllocationSize(memReq.size);
    allocInfo.setMemoryTypeIndex(memTypeIndex);

    auto mem = vk.dev.allocateMemory(allocInfo);
    return std::make_tuple(allocInfo.allocationSize, buf, mem);
  }

  auto allocateDynamicBuffer(size_t                  size,
                             vk::BufferUsageFlagBits usage,
                             vk::MemoryPropertyFlags memProps)
  {
    const size_t allocPerBuf =
        aligned(size, vk.physDevProps.limits.minUniformBufferOffsetAlignment);
    return std::tuple_cat(std::tuple(allocPerBuf),
                          allocateBuffer(vk.framesInFlight * allocPerBuf, usage, memProps));
  }

  [[nodiscard("you should keep those objects in your context")]] auto build()
  {
    std::vector<vk::DescriptorSetLayout> setLayouts;
    setLayouts.reserve(descSetLayoutBindings.size());

    for(auto& set : descSetLayoutBindings)
    {
      vk::DescriptorSetLayoutCreateInfo layoutInfo{};
      layoutInfo.setBindingCount(1);
      layoutInfo.setPBindings(&set);
      setLayouts.emplace_back(vk.dev.createDescriptorSetLayout(layoutInfo));
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setSetLayouts(setLayouts);

    if(pushConstantsRange.size != 0)
    {
      pipelineLayoutInfo.setPushConstantRangeCount(1);
      pipelineLayoutInfo.setPPushConstantRanges(&pushConstantsRange);
    }

    vk::PipelineLayout layout = vk.dev.createPipelineLayout(pipelineLayoutInfo);

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    if(vertexBinding.stride != 0)
    {
      vertexInputInfo.setVertexBindingDescriptionCount(1);
      vertexInputInfo.setPVertexBindingDescriptions(&vertexBinding);
      vertexInputInfo.setVertexAttributeDescriptionCount(1);
      vertexInputInfo.setPVertexAttributeDescriptions(&vertexAttr);
    }

    vk::PipelineColorBlendStateCreateInfo blendInfo{};
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments    = &blendAttachement;

    vk::PipelineDynamicStateCreateInfo dynamicInfo;
    dynamicInfo.setDynamicStates(dynStates);

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.pViewportState      = &viewportInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState   = &multisampleInfo;
    pipelineInfo.pDepthStencilState  = &depthStencilInfo;
    pipelineInfo.pColorBlendState    = &blendInfo;
    pipelineInfo.layout              = layout;
    pipelineInfo.renderPass          = vk.rp;
    pipelineInfo.setStages(stageInfos);
    pipelineInfo.setPVertexInputState(&vertexInputInfo);
    pipelineInfo.setPDynamicState(&dynamicInfo);

    auto cache = vk.dev.createPipelineCache(vk::PipelineCacheCreateInfo{});

    auto [res, pipeline] = vk.dev.createGraphicsPipeline(cache, pipelineInfo);
    if(res != vk::Result::eSuccess)
    {
      qFatal().nospace() << "Failed to create graphics pipeline";
    }
    return std::make_tuple(pipeline, cache, layout, setLayouts);
  }

  class OneShotCommandBuffer
  {
  public:
    OneShotCommandBuffer(VulkanContext& ctx) : ctx_{ctx}
    {
      vk::CommandBufferAllocateInfo alloc{};
      alloc.level              = vk::CommandBufferLevel::ePrimary;
      alloc.commandPool        = ctx_.commandPool;
      alloc.commandBufferCount = 1;
      commandBuffer            = ctx_.dev.allocateCommandBuffers(alloc)[0];
      vk::CommandBufferBeginInfo beginInfo{};
      beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
      commandBuffer.begin(beginInfo);
    }

    ~OneShotCommandBuffer()
    {
      commandBuffer.end();
      vk::SubmitInfo submit{};
      submit.commandBufferCount = 1;
      submit.pCommandBuffers    = &commandBuffer;
      ctx_.queue.submit(submit, {});
      ctx_.queue.waitIdle();
      ctx_.dev.freeCommandBuffers(ctx_.commandPool, ctx_.commandBuffer);
    }

    vk::CommandBuffer commandBuffer;

  private:
    VulkanContext& ctx_;
  };

  template <typename T, typename FillFn>
  inline auto createDeviceLocalVertexBuffer(size_t element_count, FillFn&& fill)
  {
    const size_t size = element_count * sizeof(T);

    // 1. Create staging buffer
    vk::BufferCreateInfo stagingInfo{};
    stagingInfo.size         = size;
    stagingInfo.usage        = vk::BufferUsageFlagBits::eTransferSrc;
    stagingInfo.sharingMode  = vk::SharingMode::eExclusive;
    vk::Buffer stagingBuffer = vk.dev.createBuffer(stagingInfo);

    vk::MemoryRequirements memReq = vk.dev.getBufferMemoryRequirements(stagingBuffer);
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize  = memReq.size;
    allocInfo.memoryTypeIndex = vk.findMemoryTypeIndex(
        memReq,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::DeviceMemory stagingMem = vk.dev.allocateMemory(allocInfo);
    vk.dev.bindBufferMemory(stagingBuffer, stagingMem, 0);

    // Map and copy
    T* mapped = reinterpret_cast<T*>(vk.dev.mapMemory(stagingMem, 0, size));
    fill(std::span<T>(mapped, element_count));
    vk.dev.unmapMemory(stagingMem);

    // 2. Create device-local buffer
    vk::BufferCreateInfo vertexInfo{};
    vertexInfo.size = size;
    vertexInfo.usage =
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    vertexInfo.sharingMode  = vk::SharingMode::eExclusive;
    vk::Buffer vertexBuffer = vk.dev.createBuffer(vertexInfo);

    memReq                   = vk.dev.getBufferMemoryRequirements(vertexBuffer);
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex =
        vk.findMemoryTypeIndex(memReq, vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::DeviceMemory vertexMem = vk.dev.allocateMemory(allocInfo);
    vk.dev.bindBufferMemory(vertexBuffer, vertexMem, 0);

    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    {
      OneShotCommandBuffer oneShot{vk};
      oneShot.commandBuffer.copyBuffer(stagingBuffer, vertexBuffer, 1, &copyRegion);
    }

    // 4. Cleanup staging
    vk.dev.destroyBuffer(stagingBuffer);
    vk.dev.freeMemory(stagingMem);

    return std::tuple{size, vertexBuffer, vertexMem};
  }

private:
  VulkanContext& vk;

public:
  vk::ShaderModule                               vertShaderModule;
  vk::ShaderModule                               geomShaderModule;
  std::vector<vk::ShaderModule>                  shaderModules;
  std::vector<vk::PipelineShaderStageCreateInfo> stageInfos;
  vk::PipelineViewportStateCreateInfo            viewportInfo{{}, 1, nullptr, 1, nullptr};

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  vk::PipelineRasterizationStateCreateInfo rasterizationInfo{{},
                                                             false,
                                                             false,
                                                             vk::PolygonMode::eFill,
                                                             vk::CullModeFlagBits::eNone,
                                                             vk::FrontFace::eCounterClockwise,
                                                             false,
                                                             0.0f,
                                                             0.0f,
                                                             0.0f,
                                                             1.0f};
  vk::PipelineMultisampleStateCreateInfo   multisampleInfo{{}, vk.rasterizationSamples};
  vk::PipelineDepthStencilStateCreateInfo  depthStencilInfo = vk.sceneDS;

  vk::PushConstantRange pushConstantsRange{};

  vk::PipelineColorBlendAttachmentState blendAttachement = []
  {
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
    return blend;
  }();

  std::vector<vk::DynamicState> dynStates = {vk::DynamicState::eViewport,
                                             vk::DynamicState::eScissor};

  vk::VertexInputBindingDescription   vertexBinding{0, 0, vk::VertexInputRate::eVertex};
  vk::VertexInputAttributeDescription vertexAttr = {
      0,                         // location
      0,                         // binding
      vk::Format::eR32G32Sfloat, // 'vertices' only has 2 floats per vertex
      0                          // offset
  };

  std::vector<vk::DescriptorSetLayoutBinding> descSetLayoutBindings;
};
