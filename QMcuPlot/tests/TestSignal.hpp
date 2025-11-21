#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include <QMcu/Plot/AbstractPlotDataProvider.hpp>

#include <ranges>

class TestSignal : public AbstractPlotDataProvider
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(uint32_t frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)
  Q_PROPERTY(uint32_t sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)
  Q_PROPERTY(double amplitude READ amplitude WRITE setAmplitude NOTIFY amplitudeChanged)

public:
  using AbstractPlotDataProvider::AbstractPlotDataProvider;
  virtual ~TestSignal() = default;

  double amplitude() const noexcept
  {
    return amplitude_;
  }

  uint32_t sampleRate() const noexcept
  {
    return sampleRate_;
  }

  uint32_t frequency() const noexcept
  {
    return frequency_;
  }
public slots:
  void setAmplitude(double amplitude)
  {
    if(amplitude != amplitude_)
    {
      amplitude_ = amplitude;
      emit amplitudeChanged(amplitude_);
    }
  }
  void setSampleRate(uint32_t sampleRate)
  {
    if(sampleRate != sampleRate_)
    {
      sampleRate_ = sampleRate;
      emit sampleRateChanged(sampleRate_);
    }
  }
  void setFrequency(uint32_t frequency)
  {
    if(frequency != frequency_)
    {
      frequency_ = frequency;
      emit frequencyChanged(frequency_);
    }
  }

signals:
  void amplitudeChanged(double);
  void sampleRateChanged(uint32_t);
  void frequencyChanged(uint32_t);

protected:
  bool initializePlotContext(PlotContext& ctx) final
  {
    const size_t n_samples = /* 2 * */ sampleRate_ * duration_;
    mapped_                = createMappedStorageBuffer<double>(n_samples);
    const double pulse     = frequency_ / double(sampleRate_);
    std::ranges::generate(mapped_,
                          [&, ii = 0] mutable
                          { return amplitude_ * std::sin(ii++ * pulse * 2.0 * std::numbers::pi); });
    return true;
  }

  UpdateRange update(PlotContext& ctx) final
  {
    return ctx.vbo.full_range();
  }

private:
  uint32_t          frequency_  = 440;
  uint32_t          sampleRate_ = 8000;
  double            amplitude_  = 1.0;
  double            duration_   = 1.0;
  std::span<double> mapped_;
};