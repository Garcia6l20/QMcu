#include <QMcu/Plot/FileIO.hpp>

#include <QDebug>
#include <QFile>

FileIO::FileIO(QObject* parent) : QObject{parent}
{
}

bool FileIO::read()
{
  if(source_.isEmpty())
  {
    qWarning() << "Source empty";
    return false;
  }
  QFile file(source_.toLocalFile());
  if(!file.exists())
  {
    qWarning().nospace() << "Does not exits: " << source_.toLocalFile();
    return false;
  }
  if(file.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&file);
    text_ = stream.readAll();
    emit textChanged(text_);
  }
  else
  {
    qWarning().nospace() << "Failed to open: " << source_.toLocalFile();
    return false;
  }
  return true;
}

bool FileIO::write()
{
  if(source_.isEmpty())
  {
    qWarning() << "Source empty";
    return false;
  }
  QFile file(source_.toLocalFile());
  if(file.open(QIODevice::WriteOnly))
  {
    QTextStream stream(&file);
    stream << text_;
  }
  else
  {
    qWarning().nospace() << "Failed to open: " << source_.toLocalFile();
    return false;
  }
  return true;
}