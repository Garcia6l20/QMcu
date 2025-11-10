#pragma once

#include <QMcu/Debug/AbstractVariableRecorder.hpp>
#include <QMcu/Debug/Variable.hpp>

#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

class BufferRecorder : public AbstractVariableRecorder
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QLineSeries* series READ series WRITE setSeries NOTIFY seriesChanged)

public:
  explicit BufferRecorder(QObject* parent = nullptr);
  virtual ~BufferRecorder() = default;

  QLineSeries* series() noexcept
  {
    return series_;
  }
  
signals:
  void seriesChanged();

public slots:
  void setSeries(QLineSeries*);

protected:
  void onValueChanged() final;

private:
  QLineSeries* series_      = nullptr;
};
