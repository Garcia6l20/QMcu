#include <QMcu/Plot/BasicRenderer.hpp>

BasicRenderer::BasicRenderer(QObject* parent) : QObject{parent}
{
}

bool BasicRenderer::initializeGL(QSize const& viewport)
{
  allocated_ = allocateGL(viewport);
  return allocated_;
}