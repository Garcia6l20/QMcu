#include <QMcu/Plot/PlotLineSeries.hpp>
#include <QMcu/Plot/VK/VulkanPipelineBuilder.hpp>

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

  auto& vk = vkContext();

  auto builder = VulkanPipelineBuilder(vk);

  builder.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eLineStrip);

  vk::PipelineRasterizationLineStateCreateInfo lineInfo{};
  lineInfo.lineRasterizationMode = vk::LineRasterizationModeEXT::eRectangularKHR;
  lineInfo.stippledLineEnable    = false;
  lineInfo.lineStippleFactor     = 1;
  lineInfo.lineStipplePattern    = 0xFFFF;
  builder.rasterizationInfo.setPNext(&lineInfo);
  builder.rasterizationInfo.setLineWidth(lineWidth_);

  switch(ctx_.data.type)
  {
    case QMetaType::Type::Float:
      builder.addStage("line-plot-series-float.vert.spv", vk::ShaderStageFlagBits::eVertex);
      break;
    case QMetaType::Type::Double:
      builder.addStage("line-plot-series-double.vert.spv", vk::ShaderStageFlagBits::eVertex);
      break;
    case QMetaType::Type::Char:
      builder.addStage("line-plot-series-i8.vert.spv", vk::ShaderStageFlagBits::eVertex);
      break;
    case QMetaType::Type::UChar:
      builder.addStage("line-plot-series-u8.vert.spv", vk::ShaderStageFlagBits::eVertex);
      break;
    case QMetaType::Type::Short:
      builder.addStage("line-plot-series-i16.vert.spv", vk::ShaderStageFlagBits::eVertex);
      break;
    case QMetaType::Type::UShort:
      builder.addStage("line-plot-series-u16.vert.spv", vk::ShaderStageFlagBits::eVertex);
      break;
    case QMetaType::Type::Int:
      builder.addStage("line-plot-series-i16.vert.spv", vk::ShaderStageFlagBits::eVertex);
      break;
    case QMetaType::Type::UInt:
      builder.addStage("line-plot-series-u16.vert.spv", vk::ShaderStageFlagBits::eVertex);
      break;
    default:
      qFatal(lcPlot) << "Unhandled data type";
      std::abort();
  }
  builder.addStage("line-plot-series.frag.spv", vk::ShaderStageFlagBits::eFragment);

  Q_ASSERT(vk.framesInFlight <= 3);

  size_t ubufSize;
  std::tie(allocPerUbuf_, ubufSize, ubuf_, ubufMem_) = vk.allocateDynamicBuffer(
      sizeof(ubo),
      vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

  vk.dev.bindBufferMemory(ubuf_, ubufMem_, 0);

  vk::DescriptorSetLayoutBinding descSetLayoutBinding{};

  // set 0
  descSetLayoutBinding.setBinding(0);
  descSetLayoutBinding.setDescriptorCount(1);
  descSetLayoutBinding.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
  descSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex
                                     | vk::ShaderStageFlagBits::eFragment
                                     | vk::ShaderStageFlagBits::eGeometry);

  builder.descSetLayoutBindings.emplace_back(descSetLayoutBinding);

  // set 1
  descSetLayoutBinding.setBinding(0);
  descSetLayoutBinding.setDescriptorCount(1);
  descSetLayoutBinding.setDescriptorType(vk::DescriptorType::eStorageBufferDynamic);
  descSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex);

  builder.descSetLayoutBindings.emplace_back(descSetLayoutBinding);

  std::vector<vk::DescriptorSetLayout> dsl;
  std::tie(pipeline_, pipelineCache_, pipelineLayout_, dsl) = builder.build();

  uniformsSetLayout_ = dsl.at(0);
  dataSetLayout_     = dsl.at(1);

  // Now just need some descriptors.
  vk::DescriptorPoolSize descPoolSizes[] = {
      {vk::DescriptorType::eUniformBufferDynamic, 1},
      {vk::DescriptorType::eStorageBufferDynamic, 1},
  };
  vk::DescriptorPoolCreateInfo descPoolInfo{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet};
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

    dev.free(ubufMem_);
    dev.destroy(ubuf_);

    dev.destroy(uniformsSetLayout_);
    dev.destroy(dataSetLayout_);

    std::array descSets{ubufDescriptor_, sbufDescriptor_};
    dev.freeDescriptorSets(descriptorPool_, descSets);
    dev.destroy(descriptorPool_);
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
