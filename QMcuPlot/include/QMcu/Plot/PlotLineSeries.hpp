#pragma once

#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QtQmlIntegration>

#include <QMcu/Plot/AbstractPlotDataProvider.hpp>
#include <QMcu/Plot/AbstractPlotSeries.hpp>
#include <QMcu/Plot/PlotContext.hpp>

#include <QColor>

class PlotLineSeries : public AbstractPlotSeries
{
  friend PlotContext;
  friend AbstractPlotDataProvider;

  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
  Q_PROPERTY(float thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
  Q_PROPERTY(float glow READ glow WRITE setGlow NOTIFY glowChanged)
  Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth FINAL)

public:
  explicit PlotLineSeries(QObject* parent = nullptr);
  virtual ~PlotLineSeries();

  QColor const& lineColor() const noexcept
  {
    return lineColor_;
  }
  float thickness() const noexcept
  {
    return thickness_;
  }
  float glow() const noexcept
  {
    return glow_;
  }
  float lineWidth() const noexcept
  {
    return lineWidth_;
  }

public slots:

  void setLineColor(QColor const& color)
  {
    if(color != lineColor_)
    {
      lineColor_ = color;
      emit lineColorChanged();
    }
  }

  void setLineWidth(float width)
  {
    lineWidth_ = width;
  }

  void setThickness(float thickness)
  {
    if(thickness != thickness_)
    {
      thickness_ = thickness;
      emit thicknessChanged(thickness_);
    }
  }

  void setGlow(float glow)
  {
    if(glow != glow_)
    {
      glow_ = glow;
      emit glowChanged(glow_);
    }
  }


signals:
  void lineColorChanged();
  void thicknessChanged(float);
  void glowChanged(float);

protected:
  bool initialize() final;
  void draw() final;
  void releaseResources() final;

private:
  void updateMetadata();

  struct UBO
  {
    glm::mat4 mvp;

    glm::mat4 dataToNdc;     // data -> NDC
    glm::mat4 viewTransform; // zoom & pan in NDC space

    glm::vec4 color; // base color

    glm::vec2 boundingSize;

    float thickness;
    float glow;

    glm::uint byteCount;    // byte count
    glm::uint byteOffset;   // byte offset
    glm::uint sampleStride; // sample stride

    glm::uint tid;
  } ubo;

  QColor    lineColor_ = Qt::red;
  float     thickness_ = 7 / 100.0f;
  float     glow_      = 1 / 100.0f;
  float     lineWidth_ = 2.0f;

  vk::Pipeline       pipeline_       = nullptr;
  vk::PipelineCache  pipelineCache_  = nullptr;
  vk::PipelineLayout pipelineLayout_ = nullptr;

  vk::Buffer       ubuf_    = nullptr;
  vk::DeviceMemory ubufMem_ = nullptr;

  vk::DescriptorSetLayout uniformsSetLayout_;
  vk::DescriptorSetLayout dataSetLayout_;

  vk::DescriptorPool descriptorPool_{};
  // vk::DescriptorSet  descriptorSets_[2]{};
  vk::DescriptorSet ubufDescriptor_{}; // = descriptorSets_[0];
  vk::DescriptorSet sbufDescriptor_{}; // = descriptorSets_[1];

  size_t allocPerUbuf_ = 0;
};
