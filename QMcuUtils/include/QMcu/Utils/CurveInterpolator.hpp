#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include <QEasingCurve>

#include <ranges>

class CurveInterpolator : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QList<QPointF> points READ points WRITE setPoints NOTIFY pointsChanged)
  Q_PROPERTY(
      QList<QPointF> controlPoints READ controlPoints NOTIFY pointsChanged)

public:
  CurveInterpolator(QObject* parent = nullptr);
  virtual ~CurveInterpolator() = default;

  QList<QPointF> const& points() const noexcept
  {
    return points_;
  }

  QList<QPointF> const& controlPoints() const noexcept
  {
    return controlPoints_;
  }

  Q_INVOKABLE QVector<double> eval(QVector<double> xs) noexcept
  {
    return xs | std::views::transform([this](double x) { return valueForProgress(x); }) |
           std::ranges::to<QVector<double>>();
  }

  Q_INVOKABLE double valueForProgress(double progress);

public slots:
  void setPoints(QList<QPointF> const& points)
  {
    if(points != points_)
    {
      points_ = points;
      update();
      emit pointsChanged();
    }
  }

signals:
  void pointsChanged();

private:
  void    update();

  QEasingCurve                              eCurve_;
  QList<QPointF>                            points_;
  QList<QPointF>                            controlPoints_;
};