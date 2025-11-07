#pragma once

#include <QtQmlIntegration>

#include <QMcu/Plot/PlotContext.hpp>

class AbstractPlotSeries;

class AbstractPlotDataProvider : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("abstract element")

  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
  using QObject::QObject;
  virtual ~AbstractPlotDataProvider() = default;

  struct UpdateRange : std::span<std::byte const>
  {
    using std::span<std::byte const>::span;

    template <typename SrcT> UpdateRange(span<SrcT> src) noexcept : span{std::as_bytes(src)}
    {
    }
  };

  QString const& name() const noexcept
  {
    return name_;
  }

signals:
  void dataChanged();
  void nameChanged(QString const&);

public slots:
  void setName(QString const& name)
  {
    if(name != name_)
    {
      name_ = name;
      emit nameChanged(name_);
    }
  }

protected:
  friend AbstractPlotSeries;

  // Called once before the renderer starts
  // Allows the provider to configure the VBO format, size, scaling, etc.
  virtual bool initializePlotContext(PlotContext& ctx) = 0;

  // Called on render thread before draw; implement to update mapped VBO range.
  virtual UpdateRange update(PlotContext& ctx) = 0;

  template <typename T> std::span<T> createMappedArrayBuffer(size_t count, uint32_t binding = 0)
  {
    return std::span(
        reinterpret_cast<T*>(createMappedBuffer(qplot::typeIdOf<T>(),
                                                count,
                                                vk::BufferUsageFlagBits::eVertexBuffer)),
        count);
  }

  std::span<std::byte> createMappedArrayBuffer(QMetaType::Type qtType, size_t count)
  {
    return qplot::visitQtType(
        qtType,
        [&]<typename T> { return std::as_writable_bytes(createMappedArrayBuffer<T>(count)); });
  }

  template <typename T> std::span<T> createMappedStorageBuffer(size_t count, uint32_t binding = 0)
  {
    return std::span(
        reinterpret_cast<T*>(createMappedBuffer(qplot::typeIdOf<T>(),
                                                count,
                                                vk::BufferUsageFlagBits::eStorageBuffer)),
        count);
  }

  std::span<std::byte> createMappedStorageBuffer(QMetaType::Type qtType, size_t count)
  {
    return qplot::visitQtType(
        qtType,
        [&]<typename T> { return std::as_writable_bytes(createMappedStorageBuffer<T>(count)); });
  }

private:
  void* createMappedBuffer(qplot::TypeId tid, size_t count, vk::BufferUsageFlagBits usage);

  AbstractPlotSeries* series_ = nullptr;
  QString             name_;
};
