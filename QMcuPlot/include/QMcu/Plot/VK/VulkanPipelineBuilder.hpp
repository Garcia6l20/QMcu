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
