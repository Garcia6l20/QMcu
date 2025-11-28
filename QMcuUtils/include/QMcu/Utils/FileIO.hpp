#pragma once

#include <QObject>
#include <QUrl>

#include <QtQmlIntegration>

class FileIO : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  explicit FileIO(QObject* parent = nullptr);

  Q_INVOKABLE static QString readText(QString const& filePath);
  Q_INVOKABLE static bool    writeText(QString const& filePath, QString const& text);

private:
};
