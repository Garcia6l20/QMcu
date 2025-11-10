#include <QMcu/Debug/AutoScale.hpp>

#include <QQuickItem>
#include <QTimer>
#include <QtGraphs/private/qgraphsview_p.h> // No QGraphsView ???

#include <QAbstractAxis>
#include <QLineSeries>

#include <ranges>

AutoScale::AutoScale(QObject* parent) : QObject(parent)
{
}

void AutoScale::addSeries(QXYSeries* series)
{
  series_.append(series);
  connect(series, &QXYSeries::pointAdded, this, &AutoScale::updateAxis);
  connect(series, &QXYSeries::pointReplaced, this, &AutoScale::updateAxis);
  connect(series, &QXYSeries::pointRemoved, this, &AutoScale::updateAxis);
}

void AutoScale::setSeries(QList<QXYSeries*> series)
{
  series_.reserve(series.size());
  for(auto s : series)
  {
    addSeries(s);
  }
}

void AutoScale::updateAxis()
{
  static auto const get_ranges = [](QXYSeries* s)
  {
    auto const& p0 = s->at(0);
    Range       rx{p0.x(), p0.x()};
    Range       ry{p0.y(), p0.y()};
    for(int ii = 1; ii < s->count(); ++ii)
    {
      auto const& p = s->at(ii);
      if(p.x() < rx.low)
      {
        rx.low = p.x();
      }
      else if(p.x() > rx.high)
      {
        rx.high = p.x();
      }
      if(p.y() < ry.low)
      {
        ry.low = p.y();
      }
      else if(p.y() > ry.high)
      {
        ry.high = p.y();
      }
    }
    return std::make_tuple(rx, ry);
  };

  auto rng =
      series_ | std::views::filter([](QXYSeries* s) { return s->isVisible() and s->count() > 0; });

  auto* const s0 = rng.front();
  if(s0->count() == 0)
  {
    return;
  }
  auto [highest_x_range, highest_y_range] = get_ranges(s0);

  for(auto* series : rng | std::views::drop(1))
  {
    auto [rx, ry] = get_ranges(series);
    if(rx.high > highest_x_range.high)
    {
      highest_x_range.high = rx.high;
    }
    if(rx.low < highest_x_range.low)
    {
      highest_x_range.low = rx.low;
    }
    if(ry.high > highest_y_range.high)
    {
      highest_y_range.high = ry.high;
    }
    if(ry.low < highest_y_range.low)
    {
      highest_y_range.low = ry.low;
    }
  }

  highest_x_range.low -= xMargin_;
  highest_x_range.high += xMargin_;

  highest_y_range.low -= yMargin_;
  highest_y_range.high += yMargin_;

  const auto graph = s0->graph();
  graph->axisX()->setRange(highest_x_range.low, highest_x_range.high);
  graph->axisY()->setRange(highest_y_range.low, highest_y_range.high);
}