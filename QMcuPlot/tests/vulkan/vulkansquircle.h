#ifndef VULKANSQUIRCLE_H
#define VULKANSQUIRCLE_H

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

class PlotScene;
class SquircleRenderer;

class VulkanSquircle : public QQuickItem
{
  Q_OBJECT
  Q_PROPERTY(qreal t READ t WRITE setT NOTIFY tChanged)
  QML_ELEMENT

public:
  VulkanSquircle();

  qreal t() const
  {
    return m_t;
  }
  void setT(qreal t);

signals:
  void tChanged();

protected:
  QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*);

private:
  void releaseResources() override;

  qreal             m_t          = 0;
  PlotScene*        m_vkRenderer = nullptr;
  SquircleRenderer* m_renderer   = nullptr;
};

#endif
