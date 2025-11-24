#pragma once

#include <QObject>
#include <QXYSeries>

#include <QtQmlIntegration>

class AutoScale : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(qreal xMargin READ xMargin WRITE setXMargin)
  Q_PROPERTY(qreal yMargin READ yMargin WRITE setYMargin)
  Q_PROPERTY(QList<QXYSeries*> series READ series WRITE setSeries)

public:
  struct Range
  {
    qreal low;
    qreal high;

    constexpr auto operator<=>(const Range& rhs) const = default;
  };

  AutoScale(QObject* parent = nullptr);
  virtual ~AutoScale() = default;

  qreal xMargin() const noexcept
  {
    return xMargin_;
  }
  qreal yMargin() const noexcept
  {
    return yMargin_;
  }
  QList<QXYSeries*> series() const noexcept
  {
    return series_;
  }

public slots:
  void setXMargin(qreal xMargin) noexcept
  {
    xMargin_ = xMargin;
  }
  void setYMargin(qreal yMargin) noexcept
  {
    yMargin_ = yMargin;
  }

  void setSeries(QList<QXYSeries*>);
  void addSeries(QXYSeries*);

private slots:
  void updateAxis();

private:
  QList<QXYSeries*> series_;
  qreal             xMargin_ = 0.0;
  qreal             yMargin_ = 0.0;
};
