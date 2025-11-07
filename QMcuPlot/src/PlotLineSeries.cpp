#include <QMcu/Plot/PlotLineSeries.hpp>

#include <Logging.hpp>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include <QSurfaceFormat>

static const QColor defaultColors[] = {
    QColorConstants::Svg::cyan,
    QColorConstants::Svg::magenta,
    QColorConstants::Svg::chartreuse,
    QColorConstants::Svg::orange,
    QColorConstants::Svg::aquamarine,
    QColorConstants::Svg::gold,
    QColorConstants::Svg::deeppink,
    QColorConstants::Svg::lawngreen,
    QColorConstants::Svg::dodgerblue,
    QColorConstants::Svg::coral,
};

PlotLineSeries::PlotLineSeries(QObject* parent)
    : AbstractPlotSeries(parent),
      lineColor_(defaultColors[(id() - 1) % (sizeof(defaultColors) / sizeof(defaultColors[0]))])
{
}

PlotLineSeries::~PlotLineSeries()
{
}

void PlotLineSeries::updateMetadata()
{
  auto& ctx = ctx_;

  const auto make_integer_binding = [&]<std::integral T>(std::type_identity<T>)
  {
    constexpr auto limits = std::numeric_limits<T>();

    ctx.data.scaleMin = limits.min();
    ctx.data.scaleMax = limits.max();

    const double min = double(limits.min());
    const double max = double(limits.max());

    ctx.data.fromNdc.setToIdentity();
    ctx.data.fromNdc.viewport(0.0, min, 1.0, max - min);
    ctx.data.toNdc = ctx.data.fromNdc.inverted();

    if(ctx.vbo.stride == 0)
    {
      ctx.vbo.stride = sizeof(T);
    }
    ctx.vbo.elem_size = sizeof(T);
  };

  switch(ctx.data.type)
  {
    case QMetaType::Float:
      if(ctx.vbo.stride == 0)
      {
        ctx.vbo.stride = sizeof(float);
      }
      ctx.vbo.elem_size = sizeof(float);
      break;
    case QMetaType::Double:
      if(ctx.vbo.stride == 0)
      {
        ctx.vbo.stride = sizeof(double);
      }
      ctx.vbo.elem_size = sizeof(double);
      break;
    case QMetaType::Int:
    {
      make_integer_binding(std::type_identity<int32_t>());
    }
    break;
      break;
    case QMetaType::UInt:
    {
      make_integer_binding(std::type_identity<uint32_t>());
    }
    break;
    case QMetaType::Short:
    {
      make_integer_binding(std::type_identity<int16_t>());
    }
    break;
    case QMetaType::UShort:
    {
      make_integer_binding(std::type_identity<uint16_t>());
    }
    break;
    case QMetaType::Char:
    {
      make_integer_binding(std::type_identity<int8_t>());
    }
    break;
    case QMetaType::UChar:
    {
      make_integer_binding(std::type_identity<uint8_t>());
    }
    break;
    default:
      qFatal() << "Unhandled type";
  }
}

bool PlotLineSeries::initialize()
{
  auto* const provider = dataProvider();
  if(not provider)
  {
    return false;
  }

  if(not initializeDataProvider())
  {
    return false;
  }

  ctx_.vbo._current_range = ctx_.vbo.full_range();

  if(ctx_.vbo._buffer == nullptr)
  {
    qFatal(lcPlot).noquote()
        << provider->metaObject()->className()
        << "::initializePlotContext: must create a buffer (ie.: with createMappedBuffer<T>(N))";
  }

  updateMetadata();

  ubo.tid = ctx_.data.type;

  auto& vk  = vkContext();
  auto& dev = vk.dev;

  auto vertShaderModule = [&]
  {
    switch(ctx_.data.type)
    {
      case QMetaType::Type::Float:
        return vk.createShaderModule("line-plot-series-float.vert.spv");
      case QMetaType::Type::Double:
        return vk.createShaderModule("line-plot-series-double.vert.spv");
      case QMetaType::Type::Char:
        return vk.createShaderModule("line-plot-series-i8.vert.spv");
      case QMetaType::Type::UChar:
        return vk.createShaderModule("line-plot-series-u8.vert.spv");
      case QMetaType::Type::Short:
        return vk.createShaderModule("line-plot-series-i16.vert.spv");
      case QMetaType::Type::UShort:
        return vk.createShaderModule("line-plot-series-u16.vert.spv");
      case QMetaType::Type::Int:
        return vk.createShaderModule("line-plot-series-i16.vert.spv");
      case QMetaType::Type::UInt:
        return vk.createShaderModule("line-plot-series-u16.vert.spv");
      default:
        qFatal(lcPlot) << "Unhandled data type";
        std::abort();
    }
  }();
  auto fragShaderModule = vk.createShaderModule("line-plot-series.frag.spv");

  Q_ASSERT(vk.framesInFlight <= 3);

  allocPerUbuf_ = aligned(sizeof(ubo), vk.physDevProps.limits.minUniformBufferOffsetAlignment);

  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.setSize(vk.framesInFlight * allocPerUbuf_);
  bufferInfo.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);
  ubuf_ = dev.createBuffer(bufferInfo);

  auto memReq       = dev.getBufferMemoryRequirements(ubuf_);
  auto memTypeIndex = vk.findMemoryTypeIndex(memReq,
                                             vk::MemoryPropertyFlagBits::eHostVisible
                                                 | vk::MemoryPropertyFlagBits::eHostCoherent);
  if(memTypeIndex == UINT32_MAX)
    qFatal("Failed to find host visible and coherent memory type");

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.setAllocationSize(vk.framesInFlight * allocPerUbuf_);
  allocInfo.setMemoryTypeIndex(memTypeIndex);
  ubufMem_ = dev.allocateMemory(allocInfo);

  dev.bindBufferMemory(ubuf_, ubufMem_, 0);

  // pipeline setup

  pipelineCache_ = dev.createPipelineCache(vk::PipelineCacheCreateInfo{});

  vk::DescriptorSetLayoutBinding    descSetLayoutBinding{};
  vk::DescriptorSetLayoutCreateInfo layoutInfo{};

  // set 0
  descSetLayoutBinding.setBinding(0);
  descSetLayoutBinding.setDescriptorCount(1);
  descSetLayoutBinding.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
  descSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex
                                     | vk::ShaderStageFlagBits::eFragment
                                     | vk::ShaderStageFlagBits::eGeometry);

  layoutInfo.setBindingCount(1);
  layoutInfo.setPBindings(&descSetLayoutBinding);
  uniformsSetLayout_ = vk.dev.createDescriptorSetLayout(layoutInfo);

  // set 1
  descSetLayoutBinding.setBinding(0);
  descSetLayoutBinding.setDescriptorCount(1);
  descSetLayoutBinding.setDescriptorType(vk::DescriptorType::eStorageBufferDynamic);
  descSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex);

  layoutInfo.setBindingCount(1);
  layoutInfo.setPBindings(&descSetLayoutBinding);
  dataSetLayout_ = vk.dev.createDescriptorSetLayout(layoutInfo);

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  vk::DescriptorSetLayout      descriptorSetLayouts[] = {uniformsSetLayout_, dataSetLayout_};
  pipelineLayoutInfo.setSetLayouts(descriptorSetLayouts);
  pipelineLayout_ = vk.dev.createPipelineLayout(pipelineLayoutInfo);

  vk::PipelineShaderStageCreateInfo stageInfo[2]{};
  stageInfo[0].setStage(vk::ShaderStageFlagBits::eVertex);
  stageInfo[0].setModule(vertShaderModule);
  stageInfo[0].setPName("main");
  stageInfo[1].setStage(vk::ShaderStageFlagBits::eFragment);
  stageInfo[1].setModule(fragShaderModule);
  stageInfo[1].setPName("main");

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
  iaInfo.topology = vk::PrimitiveTopology::eLineStrip;

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
  pipelineInfo.pColorBlendState    = &blendInfo;
  pipelineInfo.pInputAssemblyState = &iaInfo;
  pipelineInfo.pRasterizationState = &rsInfo;
  pipelineInfo.pMultisampleState   = &msInfo;
  pipelineInfo.pDepthStencilState  = &dsInfo;
  pipelineInfo.pViewportState      = &viewportInfo;
  pipelineInfo.setStages(stageInfo);
  pipelineInfo.setPVertexInputState(&vertexInputInfo);
  pipelineInfo.setPDynamicState(&dynamicInfo);

  pipelineInfo.layout = pipelineLayout_;

  pipelineInfo.renderPass = vk.rp;

  vk::Result res;
  std::tie(res, pipeline_) = dev.createGraphicsPipeline(pipelineCache_, pipelineInfo);

  dev.destroyShaderModule(vertShaderModule);
  dev.destroyShaderModule(fragShaderModule);

  if(res != vk::Result::eSuccess)
  {
    qFatal().nospace() << "Failed to create graphics pipeline: " << magic_enum::enum_name(res);
  }

  // Now just need some descriptors.
  vk::DescriptorPoolSize descPoolSizes[] = {
      {vk::DescriptorType::eUniformBufferDynamic, 1},
      {vk::DescriptorType::eStorageBufferDynamic, 1},
  };
  vk::DescriptorPoolCreateInfo descPoolInfo{};
  descPoolInfo.maxSets = 2;
  descPoolInfo.setPoolSizes(descPoolSizes);
  descriptorPool_ = vk.dev.createDescriptorPool(descPoolInfo);

  vk::DescriptorSetAllocateInfo descAllocInfo{};
  vk::WriteDescriptorSet        writeInfo{};
  vk::DescriptorBufferInfo      bufInfo{};

  // set 0: uniforms
  descAllocInfo.descriptorPool     = descriptorPool_;
  descAllocInfo.descriptorSetCount = 1;
  descAllocInfo.pSetLayouts        = &uniformsSetLayout_;
  ubufDescriptor_                  = vk.dev.allocateDescriptorSets(descAllocInfo)[0];

  writeInfo.dstSet          = ubufDescriptor_;
  writeInfo.dstBinding      = 0;
  writeInfo.descriptorCount = 1;
  writeInfo.descriptorType  = vk::DescriptorType::eUniformBufferDynamic;

  bufInfo.buffer        = ubuf_;
  bufInfo.offset        = 0; // dynamic offset is used so this is ignored
  bufInfo.range         = sizeof(ubo);
  writeInfo.pBufferInfo = &bufInfo;
  vk.dev.updateDescriptorSets(1, &writeInfo, 0, nullptr);

  // set 1: data
  descAllocInfo.descriptorPool     = descriptorPool_;
  descAllocInfo.descriptorSetCount = 1;
  descAllocInfo.pSetLayouts        = &dataSetLayout_;
  sbufDescriptor_                  = vk.dev.allocateDescriptorSets(descAllocInfo)[0];

  writeInfo.dstSet          = sbufDescriptor_;
  writeInfo.dstBinding      = 0;
  writeInfo.descriptorCount = 1;
  writeInfo.descriptorType  = vk::DescriptorType::eStorageBufferDynamic;

  bufInfo.buffer        = ctx_.vbo._buffer;
  bufInfo.offset        = 0;
  bufInfo.range         = ctx_.vbo.full_range().size_bytes();
  writeInfo.pBufferInfo = &bufInfo;
  vk.dev.updateDescriptorSets(1, &writeInfo, 0, nullptr);

  return true;
}

void PlotLineSeries::releaseResources()
{
  if(pipeline_ != nullptr)
  {
    auto& vk  = vkContext();
    auto& dev = vk.dev;
    dev.destroy(pipeline_);
    dev.destroy(pipelineLayout_);
    dev.destroy(pipelineCache_);

    dev.unmapMemory(ubufMem_);
    dev.free(ubufMem_);
    dev.destroy(ubuf_);
  }

  AbstractPlotSeries::releaseResources();
}

void PlotLineSeries::draw()
{
  if(isDirty())
  {
    const auto rng = updateDataProvider();

    if(rng.size() != ctx_.vbo._current_range.size())
    {
      ctx_.data.fromNdc.setToIdentity();
      ctx_.data.fromNdc.viewport(0.0,
                                 ctx_.data.scaleMin,
                                 (rng.size_bytes() / double(ctx_.vbo.elem_size)) - 1.0,
                                 ctx_.data.scaleMax - ctx_.data.scaleMin);
      ctx_.data.toNdc = ctx_.data.fromNdc.inverted();
      updateTransforms();
    }

    if(rng.data() != ctx_.vbo._current_range.data() or rng.size() != ctx_.vbo._current_range.size())
    {
      const GLuint offset = (rng.data() - ctx_.vbo.full_range().data()); // / ctx_.vbo.stride;

      ctx_.vbo._current_range =
          std::span<std::byte>(ctx_.vbo.full_range().data() + offset, rng.size_bytes());
    }
    setDirty(false);
  }

  const GLuint byte_offset = ctx_.vbo.current_byte_offset();
  const GLuint byte_count  = ctx_.vbo.current_byte_count();

  auto& vk  = vkContext();
  auto& dev = vk.dev;
  auto& cb  = vk.commandBuffer;

  const uint32_t ubufOffset = allocPerUbuf_ * vk.currentFrameSlot;
  {
    // static const auto y_flip = []
    // {
    //   glm::mat4 flip(1.0f);
    //   flip[1][1] = -1.0f;
    //   return flip;
    // }();
    // ubo.mvp            = y_flip * vk.modelViewProjection;
    ubo.mvp            = vk.modelViewProjection;
    ubo.boundingSize.x = vk.boundingRect.extent.width;
    ubo.boundingSize.y = vk.boundingRect.extent.height;

    ubo.dataToNdc     = toGlm(ctx_.unit.dataToNdc); // data -> NDC
    ubo.viewTransform = toGlm(ctx_.view.transform); // zoom & pan in NDC space

    ubo.color     = toGlm(lineColor_); // base color
    ubo.thickness = thickness_;
    ubo.glow      = glow_;

    ubo.byteCount    = byte_count;
    ubo.byteOffset   = byte_offset;
    ubo.sampleStride = ctx_.vbo.stride;

    auto* p = dev.mapMemory(ubufMem_, ubufOffset, allocPerUbuf_);
    memcpy(p, &ubo, sizeof(ubo));
    vk.dev.unmapMemory(ubufMem_);
  }

  // vk::MappedMemoryRange mmr{ctx_.vbo._bufferMem, 0, byte_count};
  // const auto            mmrRes = vk.dev.flushMappedMemoryRanges(1, &mmr);
  // if(mmrRes != vk::Result::eSuccess)
  // {
  //   qFatal().nospace() << "flushMappedMemoryRanges failed: " << magic_enum::enum_name(mmrRes);
  // }

  cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);

  vk::DescriptorSet         sets[]            = {ubufDescriptor_, sbufDescriptor_};
  static constexpr uint32_t sbufOffset        = 0;
  uint32_t                  dynamicOffsets[2] = {ubufOffset, sbufOffset};
  cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                        pipelineLayout_,
                        0,
                        2,
                        sets,
                        2,
                        dynamicOffsets);

  cb.draw(byte_count / ctx_.vbo.stride, 2, 0, 0);
}
