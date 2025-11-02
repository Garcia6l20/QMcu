#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include <QMcu/Plot/AbstractPlotDataProvider.hpp>

#include <ranges>

class TestSignal : public AbstractPlotDataProvider
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QVector<int> data MEMBER data_)
  Q_PROPERTY(int border MEMBER border_)

public:
  using AbstractPlotDataProvider::AbstractPlotDataProvider;
  virtual ~TestSignal() = default;

  QVector<int> data_   = {-1, 1, -1, 1, -1, 1};
  int          border_ = 1;

protected:
  bool initializePlotContext(PlotContext& ctx) final
  {
    mapped_ = createMappedArrayBuffer<int>(data_.count());
    std::ranges::copy(data_, mapped_.begin());
    return true;
  }

  UpdateRange update(PlotContext& ctx) final
  {
    return ctx.vbo.full_range();
  }

private:
  std::span<int> mapped_;
};