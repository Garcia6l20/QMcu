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

  vk::Queue                 queue;
  vk::CommandPool           commandPool;

  size_t framesInFlight;
  size_t currentFrameSlot;

  vk::Viewport viewPort; /// Window view port
  vk::Rect2D   scissor;  /// Window scissor
  vk::Rect2D   boundingRect;

  glm::mat4 modelViewProjection;

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
};
