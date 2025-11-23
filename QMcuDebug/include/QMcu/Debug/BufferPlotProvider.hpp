#pragma once

#include <QMcu/Debug/AbstractVariablePlotDataProvider.hpp>
#include <QMcu/Debug/Variable.hpp>

#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

class BufferPlotProvider : public AbstractVariablePlotDataProvider
{
  Q_OBJECT
  QML_ELEMENT

public:
  BufferPlotProvider(QObject* parent = nullptr);
  virtual ~BufferPlotProvider() = default;

protected:
  void        onValueChanged() final;
  void        onValueUnChanged() final;
  bool        initializePlotContext(PlotContext& ctx) final;
  UpdateRange update(PlotContext& ctx) final;

private:
  std::span<std::byte> mappedData_;
};
