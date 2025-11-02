#pragma once

#include <QMcu/Debug/AbstractVariableRecorder.hpp>
#include <QMcu/Debug/Variable.hpp>

#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

class ScrollRecorder : public AbstractVariableRecorder
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QLineSeries* series READ series WRITE setSeries NOTIFY seriesChanged)
  Q_PROPERTY(int sampleCount READ sampleCount WRITE setSampleCount NOTIFY sampleCountChanged)
  Q_PROPERTY(double factor READ factor WRITE setFactor NOTIFY factorChanged)
  Q_PROPERTY(XMode xMode READ xMode WRITE setXMode NOTIFY xModeChanged)

public:
  enum XMode
  {
    Index,
    Seconds,
    MiliSeconds,
  };
  Q_ENUM(XMode)

  ScrollRecorder(QObject* parent = nullptr);
  virtual ~ScrollRecorder() = default;

  QLineSeries* series() noexcept
  {
    return series_;
  }
  int sampleCount() const noexcept
  {
    return sampleCount_;
  }
  double factor() const noexcept
  {
    return factor_;
  }
  XMode xMode() const noexcept
  {
    return xMode_;
  }

signals:
  void seriesChanged();
  void factorChanged();
  void sampleCountChanged(int);
  void xModeChanged();

public slots:
  void setSeries(QLineSeries*);
  void setSampleCount(int);
  void setFactor(double factor) noexcept
  {
    if(factor_ != factor)
    {
      factor_ = factor;
      emit factorChanged();
    }
  }
  void setXMode(XMode mode) noexcept
  {
    if(mode != xMode_)
    {
      xMode_ = mode;
      emit xModeChanged();
    }
  }

protected:
  void onValueChanged() final;
  void onValueUnChanged() final;

private:
  QLineSeries* series_      = nullptr;
  int          sampleCount_ = 50;
  double       factor_      = 1.0;
  XMode        xMode_       = XMode::MiliSeconds;
};