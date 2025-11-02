#include <QMatrix4x4>

#include <QDebug>
#include <QTest>

const QRectF widget_r = {0, 0, 800, 600};

template <std::floating_point T> struct epsilon_compare
{
  T              value;
  T              epsilon = std::numeric_limits<T>::epsilon();
  constexpr bool is_default() const noexcept
  {
    return epsilon == std::numeric_limits<T>::epsilon();
  }
  explicit constexpr operator T() const noexcept
  {
    return value;
  };
  constexpr bool operator==(T actual) const noexcept
  {
    return qAbs(actual - value) <= epsilon;
  }
  constexpr auto operator[](T new_epsilon) const noexcept
  {
    return epsilon_compare{value, new_epsilon};
  }
  constexpr auto operator-() const noexcept
  {
    return epsilon_compare{-value, epsilon};
  }
};

template <std::floating_point T>
constexpr bool operator==(T actual, epsilon_compare<T> expected) noexcept
{
  return expected == actual;
}

constexpr epsilon_compare<double> operator""_ec(long double value)
{
  return epsilon_compare{double(value)};
}

template <std::floating_point T> QDebug operator<<(QDebug dbg, const epsilon_compare<T>& ec)
{
  QDebugStateSaver saver(dbg);
  dbg.nospace() << ec.value;
  if(!ec.is_default())
  {
    dbg.nospace() << "±" << ec.epsilon;
  }
  return dbg;
}

class ScalingTests : public QObject
{
  Q_OBJECT

  template <typename T> void test_impl(std::string_view id)
  {
    qDebug() << "======================" << id << "======================";

    using type                         = T;
    static constexpr auto   limits     = std::numeric_limits<type>();
    static constexpr size_t data_count = 100;

    QMatrix4x4 ndcToData;
    ndcToData.setToIdentity();

    const double min = double(limits.min());
    const double max = double(limits.max());
    const double mid = (max + min) / 2.0;

    ndcToData.viewport(0.0, min, 1.0, max - min);
    const auto dataToNdc = ndcToData.inverted();

    constexpr auto epsilon = 0.001;
    // map points
    const auto data_bot_left  = QPointF(0, min);
    const auto data_mid_point = QPointF(data_count / 2.0, mid);
    const auto data_bot_right = QPointF(data_count, max);

    const auto ndc_bot_left  = QPointF(-1, -1);
    const auto ndc_mid_point = QPointF(0, 0);
    const auto ndc_top_right = QPointF(1, 1);
    {
      qDebug() << "====================== data => ndc ======================";
      const auto bot_left  = dataToNdc.map(data_bot_left);
      const auto mid_point = dataToNdc.map(data_mid_point);
      const auto bot_right = dataToNdc.map(data_bot_right);

      qDebug() << "bot_left:  dataToNdc(" << data_bot_left << ") : " << bot_left;
      qDebug() << "mid_point: dataToNdc(" << data_mid_point << "): " << mid_point;
      qDebug() << "bot_right: dataToNdc(" << data_bot_right << "): " << bot_right;

      QCOMPARE_EQ(bot_left.y(), -1.0_ec [epsilon]);
      QCOMPARE_EQ(mid_point.y(), 0.0_ec [epsilon]);
      QCOMPARE_EQ(bot_right.y(), 1.0_ec [epsilon]);
    }
    {
      qDebug() << "====================== ndc => data ======================";
      const auto bot_left  = ndcToData.map(ndc_bot_left);
      const auto mid_point = ndcToData.map(ndc_mid_point);
      const auto top_right = ndcToData.map(ndc_top_right);

      qDebug() << "bot_left:  ndcToData(" << ndc_bot_left << ") : " << bot_left;
      qDebug() << "mid_point: ndcToData(" << ndc_mid_point << "): " << mid_point;
      qDebug() << "top_right: ndcToData(" << ndc_top_right << "): " << top_right;

      QCOMPARE_EQ(bot_left.y(), epsilon_compare{min}[epsilon]);
      QCOMPARE_EQ(mid_point.y(), epsilon_compare{mid}[epsilon]);
      QCOMPARE_EQ(top_right.y(), epsilon_compare{max}[epsilon]);
    }

    const auto x_scale_low  = QPointF(0, 0);
    const auto x_scale_high = QPointF(1, data_count);
    const auto y_scale_low  = QPointF(0, 100);
    const auto y_scale_high = QPointF(1, 200);

    QRectF dataBounds;
    dataBounds.setTop(y_scale_high.y());
    dataBounds.setBottom(y_scale_low.y());
    dataBounds.setLeft(x_scale_low.y());
    dataBounds.setRight(x_scale_high.y());

    QRectF unitBounds;
    unitBounds.setTop(y_scale_high.x());
    unitBounds.setBottom(y_scale_low.x());
    unitBounds.setLeft(x_scale_low.x());
    unitBounds.setRight(x_scale_high.x());

    auto ndcToUnit = QMatrix4x4();
    ndcToUnit.setToIdentity();
    ndcToUnit.viewport(unitBounds.left(),
                       unitBounds.bottom(),
                       unitBounds.right() - unitBounds.left(),
                       unitBounds.top() - unitBounds.bottom());

    qDebug() << "bot_left:  ndcToUnit(" << ndc_bot_left << ") : " << ndcToUnit.map(ndc_bot_left);
    qDebug() << "mid_point: ndcToUnit(" << ndc_mid_point << "): " << ndcToUnit.map(ndc_mid_point);
    qDebug() << "bot_right: ndcToUnit(" << ndc_top_right << "): " << ndcToUnit.map(ndc_top_right);

    auto unitToData = QMatrix4x4();
    unitToData.setToIdentity();
    unitToData.translate(dataBounds.left(), dataBounds.bottom());
    unitToData.scale(
        (dataBounds.right() - dataBounds.left()) / unitBounds.right() - unitBounds.left(),
        (dataBounds.top() - dataBounds.bottom()) / (unitBounds.top() - unitBounds.bottom()));

    qDebug() << "bot_left:  unitToData(" << unitBounds.bottomLeft()
             << ") : " << unitToData.map(unitBounds.bottomLeft());
    qDebug() << "mid_point: unitToData(" << unitBounds.center()
             << "): " << unitToData.map(unitBounds.center());
    qDebug() << "bot_right: unitToData(" << unitBounds.topRight()
             << "): " << unitToData.map(unitBounds.topRight());

    auto dataToUnit = unitToData.inverted();

    qDebug() << "bot_left:  dataToUnit(" << dataBounds.bottomLeft()
             << ") : " << dataToUnit.map(dataBounds.bottomLeft());
    qDebug() << "mid_point: dataToUnit(" << dataBounds.center()
             << "): " << dataToUnit.map(dataBounds.center());
    qDebug() << "bot_right: dataToUnit(" << dataBounds.topRight()
             << "): " << dataToUnit.map(dataBounds.topRight());

    auto ndcToUnitData = unitToData * ndcToUnit;
    qDebug() << "bot_left:  ndcToUnitData(" << ndc_bot_left
             << ") : " << ndcToUnitData.map(ndc_bot_left);
    qDebug() << "mid_point: ndcToUnitData(" << ndc_mid_point
             << "): " << ndcToUnitData.map(ndc_mid_point);
    qDebug() << "bot_right: ndcToUnitData(" << ndc_top_right
             << "): " << ndcToUnitData.map(ndc_top_right);

    auto unitDataToNdc = ndcToUnitData.inverted();
    qDebug() << "bot_left:  unitDataToNdc(" << dataBounds.bottomLeft()
             << ") : " << unitDataToNdc.map(dataBounds.bottomLeft());
    qDebug() << "mid_point: unitDataToNdc(" << dataBounds.center()
             << "): " << unitDataToNdc.map(dataBounds.center());
    qDebug() << "bot_right: unitDataToNdc(" << dataBounds.topRight()
             << "): " << unitDataToNdc.map(dataBounds.topRight());

    auto viewPortToNdc = QMatrix4x4();
    viewPortToNdc.setToIdentity();
  }

  QMatrix4x4 rectTransform(const QRectF& src, const QRectF& dst)
  {

    // Compute scale factors
    const qreal sx = dst.width() / src.width();
    const qreal sy = dst.height() / src.height();

    // Compute translation
    const qreal tx = dst.x() - src.x() * sx;
    const qreal ty = dst.y() - src.y() * sy;

    QMatrix4x4 m;
    
    // Apply transform
    m.translate(tx, ty);
    m.scale(sx, sy);

    return m;
  }

private slots:

  void test_basic()
  {
    const auto dataRect   = QRectF{QPointF{0, 10}, QPointF{50, -10}};
    const auto unitRect   = QRectF{QPointF{-1, 10}, QPointF{0, -10}};
    const auto dataToUnit = rectTransform(dataRect, unitRect);
    qDebug() << "dataToUnit(" << dataRect << ") = " << dataToUnit.mapRect(dataRect);
    const auto mapDataToUnit = [&](QPointF const& p)
    {
      auto result = dataToUnit.map(p);
      qDebug() << "dataToUnit(" << p << ") = " << dataToUnit.map(p);
      return result;
    };
    mapDataToUnit(QPointF{0, 0});
    mapDataToUnit(QPointF{0, 10});
    mapDataToUnit(QPointF{0, -10});
    mapDataToUnit(QPointF{50, 0});
    mapDataToUnit(QPointF{50, 10});
    mapDataToUnit(QPointF{50, -10});
  }

  void test_int16()
  {
    test_impl<int16_t>("int16_t");
  }
  void test_uint16()
  {
    test_impl<uint16_t>("uint16_t");
  }
};

QTEST_GUILESS_MAIN(ScalingTests)
#include "test-scaling.moc"