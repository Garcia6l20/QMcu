// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QOffscreenSurface>
#include <QWindow>
#include <rhi/qrhi.h>

class RhiWindow : public QWindow
{
public:
  RhiWindow();
  void releaseSwapChain();

protected:
  std::unique_ptr<QRhi>                     m_rhi;
  std::unique_ptr<QRhiSwapChain>            m_sc;
  std::unique_ptr<QRhiRenderBuffer>         m_ds;
  std::unique_ptr<QRhiRenderPassDescriptor> m_rp;
  bool                                      m_hasSwapChain = false;
  QMatrix4x4                                m_viewProjection;

private:
  void init();
  void resizeSwapChain();
  void render();

  void exposeEvent(QExposeEvent*) override;
  bool event(QEvent*) override;

  bool m_initialized  = false;
  bool m_notExposed   = false;
  bool m_newlyExposed = false;
};

#endif
