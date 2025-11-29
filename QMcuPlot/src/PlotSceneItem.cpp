#include <QMcu/Plot/PlotSceneItem.hpp>

PlotSceneItem::PlotSceneItem(QObject* parent) : QObject{parent}
{
}

bool PlotSceneItem::initialize(VulkanContext& ctx)
{
  if(initialized_)
  {
    return true;
  }
  vk_ = &ctx;

  initialized_ = doInitialize();

  return initialized_;
}

void PlotSceneItem::release()
{
  if(not initialized_)
  {
    return;
  }

  doReleaseResources();
  initialized_ = false;
}