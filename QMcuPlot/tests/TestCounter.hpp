#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include <QMcu/Plot/AbstractPlotDataProvider.hpp>

#include <ranges>

class TestCounter : public AbstractPlotDataProvider
{
  Q_OBJECT
  QML_ELEMENT

  using base_type = int32_t;

  Q_PROPERTY(int initial MEMBER initial_)
  Q_PROPERTY(int low MEMBER low_)
  Q_PROPERTY(int high MEMBER high_)
  Q_PROPERTY(int border MEMBER high_)
  Q_PROPERTY(int increment MEMBER increment_)
  Q_PROPERTY(int size MEMBER size_)
  Q_PROPERTY(int shift MEMBER shift_)

public:
  using AbstractPlotDataProvider::AbstractPlotDataProvider;
  virtual ~TestCounter() = default;

  int initial_   = 0;
  int low_       = -100;
  int high_      = 100;
  int border_    = 10;
  int increment_ = 1;
  int size_      = 100;
  int shift_     = 10;

protected:
  bool initializePlotContext(PlotContext& ctx) final
  {
    data_ = createMappedStorageBuffer<base_type>(size_);
    std::ranges::fill(data_, initial_);
    // ctx.data.scaleMin = low_ - border_;
    // ctx.data.scaleMax = high_ + border_;
    // std::ranges::generate(data_, [ii = 0] mutable { return ii++; });
    return true;
  }

  UpdateRange update(PlotContext& ctx) final
  {
    using namespace std::views;
    using namespace std::ranges;

    auto& rng = data_;

    auto last = rng.back();
    std::shift_left(begin(rng), end(rng), shift_);
    for(auto& i : rng | drop(rng.size() - shift_))
    {
      i = last + increment_;
      if(i > high_)
      {
        i = low_;
      }
      else if(i < low_)
      {
        i = high_;
      }
      last = i;
    }
    return std::as_bytes(data_);
  }

private:
  std::span<base_type> data_;
};