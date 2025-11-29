#pragma once

#include <vulkan/vulkan.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <QColor>
#include <QFile>
#include <QMatrix4x4>
#include <QRectF>
#include <QSurfaceFormat>

static constexpr size_t aligned(size_t v, size_t alignment) noexcept
{
  return (v + alignment - 1) & ~(alignment - 1);
}

static constexpr glm::mat4 toGlm(QMatrix4x4 const& m)
{
  return glm::make_mat4(m.constData());
}

static constexpr glm::vec4 toGlm(QColor const& c)
{
  return glm::vec4{c.redF(), c.greenF(), c.blueF(), c.alphaF()};
}

struct VulkanContext
{
  vk::PhysicalDevice                 phyDev;
  vk::Device                         dev;
  vk::RenderPass                     rp;
  vk::PhysicalDeviceProperties       physDevProps;
  vk::PhysicalDeviceMemoryProperties physDevMemProps;
  vk::CommandBuffer                  commandBuffer;

  vk::Queue       queue;
  vk::CommandPool commandPool;

  size_t framesInFlight;
  size_t currentFrameSlot;

  vk::Viewport viewPort; /// Window view port
  vk::Rect2D   scissor;  /// Window scissor
  vk::Rect2D   boundingRect;

  glm::mat4 modelViewProjection;

  static vk::PipelineCache pipelineCache;

  vk::SampleCountFlagBits rasterizationSamples = []
  {
    auto const fmt = QSurfaceFormat::defaultFormat();
    switch(fmt.samples())
    {
      case 1:
        return vk::SampleCountFlagBits::e1;
      case 2:
        return vk::SampleCountFlagBits::e1;
      case 4:
        return vk::SampleCountFlagBits::e4;
      case 8:
        return vk::SampleCountFlagBits::e8;
      default:
        return vk::SampleCountFlagBits::e16; // we use 16 samples at max
    }
  }();

  vk::PipelineDepthStencilStateCreateInfo sceneDS;
  vk::StencilOpState                      stencilTest;

  inline QByteArray getShaderData(QString const& resourceName)
  {
    QFile f;
    if(not resourceName.startsWith(":"))
    {
      f.setFileName(QString(":/qmcu/plot/shaders/%1").arg(resourceName));
    }
    else
    {
      f.setFileName(resourceName);
    }
    if(!f.open(QIODevice::ReadOnly))
    {
      qFatal("Failed to read shader %s", qPrintable(resourceName));
    }

    return f.readAll();
  }

  inline vk::ShaderModule createShaderModule(QString const& resourceName)
  {
    const auto data = getShaderData(resourceName);
    return dev.createShaderModule(
        vk::ShaderModuleCreateInfo{{},
                                   size_t(data.size()),
                                   reinterpret_cast<const quint32*>(data.constData())});
  }

  uint32_t findMemoryTypeIndex(vk::MemoryRequirements const& req, vk::MemoryPropertyFlags flags)
  {
    for(uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i)
    {
      if(req.memoryTypeBits & (1u << i))
      {
        if((physDevMemProps.memoryTypes[i].propertyFlags & flags) == flags)
        {
          return i;
        }
      }
    }
    return UINT32_MAX;
  };

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

  [[nodiscard]] inline auto
      allocateBuffer(size_t                  size,
                     vk::BufferUsageFlags    usage,
                     vk::MemoryPropertyFlags memProps,
                     vk::SharingMode         sharingMode = vk::SharingMode::eExclusive)
  {
    const vk::BufferCreateInfo bufferInfo{{}, size, usage, sharingMode};

    auto buf          = dev.createBuffer(bufferInfo);
    auto memReq       = dev.getBufferMemoryRequirements(buf);
    auto memTypeIndex = findMemoryTypeIndex(memReq, memProps);
    if(memTypeIndex == UINT32_MAX)
      qFatal("Failed to find host visible and coherent memory type");

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.setAllocationSize(memReq.size);
    allocInfo.setMemoryTypeIndex(memTypeIndex);

    auto mem = dev.allocateMemory(allocInfo);
    return std::make_tuple(allocInfo.allocationSize, buf, mem);
  }

  [[nodiscard]] inline auto
      allocateDynamicBuffer(size_t                  size,
                            vk::BufferUsageFlags    usage,
                            vk::MemoryPropertyFlags memProps,
                            vk::SharingMode         sharingMode = vk::SharingMode::eExclusive)
  {
    const size_t allocPerBuf = aligned(size, physDevProps.limits.minUniformBufferOffsetAlignment);
    return std::tuple_cat(
        std::tuple(allocPerBuf),
        allocateBuffer(framesInFlight * allocPerBuf, usage, memProps, sharingMode));
  }

  template <typename T, typename Fill>
  [[nodiscard]] inline auto createDeviceLocalVertexBuffer(size_t element_count, Fill&& fill)
  {
    const size_t size = element_count * sizeof(T);

    // 1. Create staging buffer
    auto const [stagingSize, stagingBuffer, stagingMem] = allocateBuffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    dev.bindBufferMemory(stagingBuffer, stagingMem, 0);

    // 2. Map and fill staging buffer
    T* mapped = reinterpret_cast<T*>(dev.mapMemory(stagingMem, 0, size));
    fill(std::span<T>(mapped, element_count));
    dev.unmapMemory(stagingMem);

    // 3. Create device-local buffer
    auto const [vertexSize, vertexBuffer, vertexMem] = allocateBuffer(
        size,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    dev.bindBufferMemory(vertexBuffer, vertexMem, 0);

    // 4. Copy staging to device-local buffer
    {
      vk::BufferCopy copyRegion{};
      copyRegion.size = size;
      OneShotCommandBuffer oneShot{*this};
      oneShot.commandBuffer.copyBuffer(stagingBuffer, vertexBuffer, 1, &copyRegion);
    }

    // 5. Cleanup staging
    dev.destroyBuffer(stagingBuffer);
    dev.freeMemory(stagingMem);

    return std::tuple{size, vertexBuffer, vertexMem};
  }
};
