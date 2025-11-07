// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// Qt uses Dynamic Dispatcher...
// Either:
//  - use Qt's dispatcher (how to get it ???)
//  - include vulkan.hpp first !
#include <QMcu/Plot/PlotGrid.hpp>
#include <QMcu/Plot/PlotScene.hpp>
#include <QMcu/Plot/PlotSceneItem.hpp>

#include <QtCore/QRunnable>
#include <QtQuick/QQuickWindow>
#include <vulkansquircle.h>
#include <vulkansquirclerenderer.h>

#include <QFile>
#include <QVulkanFunctions>
#include <QVulkanInstance>

VulkanSquircle::VulkanSquircle()
{
  setFlag(ItemHasContents, true);
  connect(this, &VulkanSquircle::tChanged, this, &VulkanSquircle::update);
}

void VulkanSquircle::setT(qreal t)
{
  if(t == m_t)
    return;
  m_t = t;
  emit tChanged();
}

// The safe way to release custom graphics resources is to both connect to
// sceneGraphInvalidated() and implement releaseResources(). To support
// threaded render loops the latter performs the SquircleRenderer destruction
// via scheduleRenderJob(). Note that the VulkanSquircle may be gone by the time
// the QRunnable is invoked.

// void VulkanSquircle::cleanup()
// {
//   delete m_renderer;
//   m_renderer = nullptr;
// }

class CleanupJob : public QRunnable
{
public:
  CleanupJob(SquircleRenderer* renderer) : m_renderer(renderer)
  {
  }
  void run() override
  {
    delete m_renderer;
  }

private:
  SquircleRenderer* m_renderer;
};

void VulkanSquircle::releaseResources()
{
  window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
  m_renderer = nullptr;
}

QSGNode* VulkanSquircle::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
  PlotScene* node = static_cast<PlotScene*>(old);
  if(not node)
  {
    node = m_vkRenderer = new PlotScene(window());
    m_renderer          = new SquircleRenderer;
    m_vkRenderer->addRenderer(m_renderer);
    // m_vkRenderer->addRenderer(new PlotGrid(this));

    window()->setColor(QColorConstants::Black);
  }
  m_vkRenderer->setBoundingRect(mapRectToScene(boundingRect()));
  m_renderer->setT(m_t);
  return node;
}
