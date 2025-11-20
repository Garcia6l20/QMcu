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

void VulkanSquircle::releaseResources()
{
  m_renderer = nullptr;
  m_vkRenderer = nullptr;
}

QSGNode* VulkanSquircle::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
  PlotScene* node = static_cast<PlotScene*>(old);
  if(not node)
  {
    node = m_vkRenderer = new PlotScene(window());
    m_renderer          = new SquircleRenderer;
    m_vkRenderer->addRenderer(m_renderer);
    // m_vkRenderer->addRenderer(new PlotGrid);

    window()->setColor(QColorConstants::Black);
  }
  m_vkRenderer->setBoundingRect(mapRectToScene(boundingRect()));
  m_renderer->setT(m_t);
  return node;
}
