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

protected:
  virtual bool initialize()       = 0;
  virtual void draw()             = 0;
  virtual void releaseResources() = 0;

  bool hasContext() const noexcept
  {
    return vk_ != nullptr;
  }

  VulkanContext& vkContext() const noexcept
  {
    return *vk_;
  }

private:
  friend PlotScene;

  VulkanContext* vk_ = nullptr;

  bool dirty_       = true;
  bool initialized_ = false;
};
