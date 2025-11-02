#pragma once

#include <QObject>
#include <QtQmlIntegration>

class StLinkProbe : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QString serial READ serial WRITE setSerial NOTIFY serialChanged)
  Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)

public:
  StLinkProbe(QObject* parent = nullptr);
  virtual ~StLinkProbe();

  static StLinkProbe* instance() noexcept
  {
    return instance_;
  }

  QString const& serial() const noexcept
  {
    return serial_;
  }
  int speed() const noexcept
  {
    return speed_;
  }

  using address_t = uint64_t;

  bool read(address_t address, std::span<std::byte> data);

  template <typename T> T read(address_t address)
  {
    T value;
    read(address, std::as_writable_bytes(std::span{&value, 1}));
    return value;
  }

public slots:
  void setSerial(QString const& serial)
  {
    if(serial != serial_)
    {
      serial_ = serial;
      emit serialChanged();
    }
  }
  void setSpeed(int speed)
  {
    if(speed != speed_)
    {
      speed_ = speed;
      emit speedChanged();
    }
  }

signals:
  void serialChanged();
  void speedChanged();

private:
  static StLinkProbe* instance_;
  QString             serial_;
  int                 speed_ = 1000;

  struct _stlink* sl_ = nullptr;
};
