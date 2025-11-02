#include <QMcu/Plot/CurveInterpolator.hpp>

CurveInterpolator::CurveInterpolator(QObject* parent)
    : QObject(parent), eCurve_(QEasingCurve::Type::BezierSpline)
{
}

double CurveInterpolator::valueForProgress(double progress)
{
  return eCurve_.valueForProgress(progress);
}

QList<qreal> firstControlPoints(const QList<qreal>& list)
{
  QList<qreal> result;

  int count = list.size();
  result.resize(count);
  result[0] = list[0] / 2.0;

  QList<qreal> temp;
  temp.resize(count);
  temp[0] = 0;

  qreal b = 2.0;

  for(int i = 1; i < count; i++)
  {
    temp[i]   = 1 / b;
    b         = (i < count - 1 ? 4.0 : 3.5) - temp[i];
    result[i] = (list[i] - result[i - 1]) / b;
  }

  for(int i = 1; i < count; i++)
    result[count - i - 1] -= temp[count - i] * result[count - i];

  return result;
}

void calculateControlPoints(const QList<QPointF>& points, QList<QPointF>& controlPoints)
{
  controlPoints.resize(points.size() * 2 - 2);

  int n = points.size() - 1;

  if(n == 1)
  {
    // for n==1
    controlPoints[0].setX((2 * points[0].x() + points[1].x()) / 3);
    controlPoints[0].setY((2 * points[0].y() + points[1].y()) / 3);
    controlPoints[1].setX(2 * controlPoints[0].x() - points[0].x());
    controlPoints[1].setY(2 * controlPoints[0].y() - points[0].y());
    return;
  }

  // Calculate first Bezier control points
  // Set of equations for P0 to Pn points.
  //
  //  |   2   1   0   0   ... 0   0   0   ... 0   0   0   |   |   P1_1    |   |   P0 + 2 * P1 | | 1
  //  4   1   0   ... 0   0   0   ... 0   0   0   |   |   P1_2    |   |   4 * P1 + 2 * P2         |
  //  |   0   1   4   1   ... 0   0   0   ... 0   0   0   |   |   P1_3    |   |   4 * P2 + 2 * P3 |
  //  |   .   .   .   .   .   .   .   .   .   .   .   .   |   |   ...     |   |   ... | |   0   0 0
  //  0   ... 1   4   1   ... 0   0   0   | * |   P1_i    | = |   4 * P(i-1) + 2 * Pi     | |   . .
  //  .   .   .   .   .   .   .   .   .   .   |   |   ...     |   |   ...                     | | 0
  //  0   0   0   0   0   0   0   ... 1   4   1   |   |   P1_(n-1)|   |   4 * P(n-2) + 2 * P(n-1) |
  //  |   0   0   0   0   0   0   0   0   ... 0   2   7   |   |   P1_n    |   |   8 * P(n-1) + Pn |
  //
  QList<qreal> list;
  list.resize(n);

  list[0] = points[0].x() + 2 * points[1].x();

  for(int i = 1; i < n - 1; ++i)
    list[i] = 4 * points[i].x() + 2 * points[i + 1].x();

  list[n - 1] = (8 * points[n - 1].x() + points[n].x()) / 2.0;

  const QList<qreal> xControl = firstControlPoints(list);

  list[0] = points[0].y() + 2 * points[1].y();

  for(int i = 1; i < n - 1; ++i)
    list[i] = 4 * points[i].y() + 2 * points[i + 1].y();

  list[n - 1] = (8 * points[n - 1].y() + points[n].y()) / 2.0;

  const QList<qreal> yControl = firstControlPoints(list);

  for(int i = 0, j = 0; i < n; ++i, ++j)
  {
    controlPoints[j].setX(xControl[i]);
    controlPoints[j].setY(yControl[i]);

    j++;

    if(i < n - 1)
    {
      controlPoints[j].setX(2 * points[i + 1].x() - xControl[i + 1]);
      controlPoints[j].setY(2 * points[i + 1].y() - yControl[i + 1]);
    }
    else
    {
      controlPoints[j].setX((points[n].x() + xControl[n - 1]) / 2);
      controlPoints[j].setY((points[n].y() + yControl[n - 1]) / 2);
    }
  }
}

void CurveInterpolator::update()
{
  eCurve_ = QEasingCurve{QEasingCurve::Type::BezierSpline};

  calculateControlPoints(points_, controlPoints_);

  for(int i = 0; i < points_.size() - 1; ++i)
  {
    eCurve_.addCubicBezierSegment(controlPoints_.at(i * 2),
                                  controlPoints_.at((i * 2) + 1),
                                  points_.at(i + 1));
  }
}