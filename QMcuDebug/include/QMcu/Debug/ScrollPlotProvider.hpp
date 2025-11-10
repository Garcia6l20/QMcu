#pragma once

#include <QMcu/Debug/AbstractVariablePlotDataProvider.hpp>
#include <QMcu/Debug/Variable.hpp>

#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

class ScrollPlotProvider : public AbstractVariablePlotDataProvider
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int sampleCount READ sampleCount WRITE setSampleCount NOTIFY sampleCountChanged)

public:
  ScrollPlotProvider(QObject* parent = nullptr);
  virtual ~ScrollPlotProvider() = default;

  int sampleCount() const noexcept
  {
    return sampleCount_;
  }

signals:
  void sampleCountChanged(int);

public slots:
  void setSampleCount(int);

protected:
  void        onValueChanged() final;
  void        onValueUnChanged() final;
  bool        initializePlotContext(PlotContext& ctx) final;
  UpdateRange update(PlotContext& ctx) final;

private:
  void pushLastValue();

  int                  sampleCount_ = 50;
  std::span<std::byte> mappedData_;
  size_t               currentOffset_ = 0;
  size_t               readIndex_ = 0;
  QVariant             lastValue_;
};
