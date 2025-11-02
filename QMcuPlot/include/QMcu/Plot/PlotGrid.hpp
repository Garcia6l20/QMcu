#pragma once

#include <QMcu/Plot/BasicRenderer.hpp>

#include <QColor>
#include <QOpenGLShaderProgram>

class PlotRenderer;

class PlotGrid : public BasicRenderer
{
  friend PlotRenderer;

  Q_OBJECT

  Q_PROPERTY(uint32_t ticks READ ticks WRITE setTicks NOTIFY ticksChanged)

public:
  explicit PlotGrid(QObject* parent = nullptr);

  uint32_t ticks() const noexcept
  {
    return ticks_;
  }

public slots:
  void setTicks(uint32_t ticks)
  {
    if(ticks != ticks_)
    {
      ticks_ = ticks;
      setDirty();
      emit ticksChanged(ticks_);
    }
  }

signals:
  void ticksChanged(uint32_t);

protected:
  bool allocateGL(QSize const& viewport) final;
  void draw() final;

private:
  std::unique_ptr<QOpenGLShaderProgram> compute_;
  std::unique_ptr<QOpenGLShaderProgram> program_;
  uint32_t                              ticks_ = 5;
  QColor                                color_ = Qt::GlobalColor::lightGray;
  GLuint                                glHandle_ = 0;
};
