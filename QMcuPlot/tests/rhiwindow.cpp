// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "rhiwindow.h"
#include <QFile>
#include <QPainter>
#include <QPlatformSurfaceEvent>
#include <rhi/qshader.h>

RhiWindow::RhiWindow()
{
  setSurfaceType(VulkanSurface);
}

//! [expose]
void RhiWindow::exposeEvent(QExposeEvent*)
{
  // initialize and start rendering when the window becomes usable for graphics purposes
  if(isExposed() && !m_initialized)
  {
    init();
    resizeSwapChain();
    m_initialized = true;
  }

  const QSize surfaceSize = m_hasSwapChain ? m_sc->surfacePixelSize() : QSize();

  // stop pushing frames when not exposed (or size is 0)
  if((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_initialized && !m_notExposed)
    m_notExposed = true;

  // Continue when exposed again and the surface has a valid size. Note that
  // surfaceSize can be (0, 0) even though size() reports a valid one, hence
  // trusting surfacePixelSize() and not QWindow.
  if(isExposed() && m_initialized && m_notExposed && !surfaceSize.isEmpty())
  {
    m_notExposed   = false;
    m_newlyExposed = true;
  }

  // always render a frame on exposeEvent() (when exposed) in order to update
  // immediately on window resize.
  if(isExposed() && !surfaceSize.isEmpty())
    render();
}
//! [expose]

//! [event]
bool RhiWindow::event(QEvent* e)
{
  switch(e->type())
  {
    case QEvent::UpdateRequest:
      render();
      break;

    case QEvent::PlatformSurface:
      // this is the proper time to tear down the swapchain (while the native window and surface are
      // still around)
      if(static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType()
         == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
        releaseSwapChain();
      break;

    default:
      break;
  }

  return QWindow::event(e);
}

void RhiWindow::init()
{
  QRhiVulkanInitParams params;
  params.inst   = vulkanInstance();
  params.window = this;
  m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));

  if(!m_rhi)
    qFatal("Failed to create RHI backend");

  m_sc.reset(m_rhi->newSwapChain());
  m_ds.reset(
      m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                             QSize(), // no need to set the size here, due to UsedWithSwapChainOnly
                             1,
                             QRhiRenderBuffer::UsedWithSwapChainOnly));
  m_sc->setWindow(this);
  m_sc->setDepthStencil(m_ds.get());
  m_rp.reset(m_sc->newCompatibleRenderPassDescriptor());
  m_sc->setRenderPassDescriptor(m_rp.get());
}

void RhiWindow::resizeSwapChain()
{
  m_hasSwapChain = m_sc->createOrResize(); // also handles m_ds

  const QSize outputSize = m_sc->currentPixelSize();
  m_viewProjection       = m_rhi->clipSpaceCorrMatrix();
  m_viewProjection.perspective(45.0f,
                               outputSize.width() / (float)outputSize.height(),
                               0.01f,
                               1000.0f);
  m_viewProjection.translate(0, 0, -4);
}

void RhiWindow::releaseSwapChain()
{
  if(m_hasSwapChain)
  {
    m_hasSwapChain = false;
    m_sc->destroy();
  }
}

void RhiWindow::render()
{
  if(!m_hasSwapChain || m_notExposed)
    return;

  // If the window got resized or newly exposed, resize the swapchain. (the
  // newly-exposed case is not actually required by some platforms, but is
  // here for robustness and portability)
  //
  // This (exposeEvent + the logic here) is the only safe way to perform
  // resize handling. Note the usage of the RHI's surfacePixelSize(), and
  // never QWindow::size(). (the two may or may not be the same under the hood,
  // depending on the backend and platform)
  //
  if(m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed)
  {
    resizeSwapChain();
    if(!m_hasSwapChain)
      return;
    m_newlyExposed = false;
  }
  //! [render-resize]

  //! [beginframe]
  QRhi::FrameOpResult result = m_rhi->beginFrame(m_sc.get());
  if(result == QRhi::FrameOpSwapChainOutOfDate)
  {
    resizeSwapChain();
    if(!m_hasSwapChain)
      return;
    result = m_rhi->beginFrame(m_sc.get());
  }
  if(result != QRhi::FrameOpSuccess)
  {
    qWarning("beginFrame failed with %d, will retry", result);
    requestUpdate();
    return;
  }

  m_rhi->endFrame(m_sc.get());

  // Always request the next frame via requestUpdate(). On some platforms this is backed
  // by a platform-specific solution, e.g. CVDisplayLink on macOS, which is potentially
  // more efficient than a timer, queued metacalls, etc.
  requestUpdate();
}
