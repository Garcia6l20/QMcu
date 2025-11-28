#include <QMcu/Utils/FileIO.hpp>

#include <QDebug>
#include <QFile>

FileIO::FileIO(QObject* parent) : QObject{parent}
{
}

QString FileIO::readText(QString const& filePath)
{
  QFile file(filePath);
  if(!file.exists())
  {
    qWarning().nospace() << "Does not exits: " << filePath;
    return {};
  }
  if(file.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&file);
    return stream.readAll();
  }
  else
  {
    qWarning().nospace() << "Failed to open: " << filePath;
    return {};
  }
}

bool FileIO::writeText(QString const& filePath, QString const& text)
{
  QFile file(filePath);
  if(file.open(QIODevice::WriteOnly))
  {
    QTextStream stream(&file);
    stream << text;
    return true;
  }
  else
  {
    qWarning().nospace() << "Failed to open: " << filePath;
    return false;
  }
}