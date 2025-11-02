#include <QMcu/Plot/PlotRenderer.hpp>
#include <QOpenGLFramebufferObjectFormat>

#include <QMcu/Plot/Plot.hpp>

QElapsedTimer PlotRenderer::sTimer_ = []
{
  QElapsedTimer t;
  t.start();
  return t;
}();

PlotRenderer::PlotRenderer(Plot* plot) : plot_(plot)
{
}

QOpenGLFramebufferObject* PlotRenderer::createFramebufferObject(const QSize& size)
{
  initializeOpenGLFunctions();

  QOpenGLFramebufferObjectFormat fmt;
  fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  fmt.setSamples(4);
  viewport_ = size;
  return new QOpenGLFramebufferObject(size, fmt);
}

void PlotRenderer::synchronize(QQuickFramebufferObject* item)
{
  // Not used yet — could sync updated series list or camera params
  Q_UNUSED(item);
}

void PlotRenderer::render()
{
  glViewport(0, 0, viewport_.width(), viewport_.height());
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const auto t = sTimer_.elapsed() / 1000.0;

  if(!plot_->grid_->isAllocated())
  {
    plot_->grid_->initializeGL(viewport_);
  }
  plot_->grid_->draw();

  for(auto& s : plot_->series_)
  {
    auto& vbo = s->context().vbo;
    if(!s->isAllocated())
    {
      if(not s->initializeGL(viewport_))
      {
        return;
      }
    }
    s->draw();
  }
}
