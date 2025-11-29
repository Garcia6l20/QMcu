#pragma once

#include <QObject>

#include <QMcu/Plot/PlotContext.hpp>
#include <QMcu/Plot/VK/VulkanContext.hpp>

class PlotScene;

class PlotSceneItem : public QObject
{
public:
  explicit PlotSceneItem(QObject* parent = nullptr);

  bool isDirty() const noexcept
  {
    return dirty_;
  }

  void setDirty(bool dirty = true)
  {
    dirty_ = dirty;
  }

  bool isInitialized() const noexcept
  {
    return initialized_;
  }

  bool initialize(VulkanContext& ctx);
  void release();

  inline void draw()
  {
    doDraw();
  }

protected:
  virtual bool    doInitialize()       = 0;
  virtual void    doDraw()             = 0;
  virtual void    doReleaseResources() = 0;

  bool hasContext() const noexcept
  {
    return vk_ != nullptr;
  }

  VulkanContext& vkContext() const noexcept
  {
    return *vk_;
  }

  vk::Pipeline       pipeline_       = nullptr;
  vk::PipelineLayout pipelineLayout_ = nullptr;

private:
  VulkanContext* vk_ = nullptr;

  bool dirty_       = true;
  bool initialized_ = false;
};
