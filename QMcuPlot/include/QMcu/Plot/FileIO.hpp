#pragma once

#include <QObject>
#include <QUrl>

#include <QtQmlIntegration>

class FileIO : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
  explicit FileIO(QObject* parent = nullptr);

  QUrl const& source() const noexcept
  {
    return source_;
  }
  QString const& text() const noexcept
  {
    return text_;
  }

  Q_INVOKABLE bool read();
  Q_INVOKABLE bool write();

signals:
  void sourceChanged(QUrl const&);
  void textChanged(QString const&);

public slots:
  void setSource(QUrl const& s)
  {
    if(s != source_)
    {
      source_ = s;
      emit sourceChanged(source_);
    }
  }
  void setText(QString const& s)
  {
    if(s != text_)
    {
      text_ = s;
      emit textChanged(text_);
    }
  }

private:
  QUrl    source_;
  QString text_;
};
