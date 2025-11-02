#pragma once

#include <QObject>

#include <QMcu/Plot/PlotContext.hpp>

class BasicRenderer : public QObject, public QOpenGLFunctions_4_5_Core
{
public:
  explicit BasicRenderer(QObject* parent = nullptr);

  bool isDirty() const noexcept
  {
    return dirty_;
  }

  void setDirty(bool dirty = true)
  {
    dirty_ = dirty;
  }

  bool isAllocated() const noexcept
  {
    return allocated_;
  }

  bool initializeGL(QSize const& viewport);

protected:
  virtual bool allocateGL(QSize const& viewport) = 0;
  virtual void draw()                            = 0;

private:
  bool dirty_     = true;
  bool allocated_ = false;
};
