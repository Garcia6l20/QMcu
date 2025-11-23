#pragma once

#include <QObject>
#include <QtQmlIntegration>

enum LogLevel
{
  Debug,
  Info,
  Warning,
  Critical,
  Fatal,
};

struct LogEntry
{
  Q_GADGET
  QML_NAMED_ELEMENT(logEntry)
  QML_UNCREATABLE("Created by Logger")

  Q_PROPERTY(LogLevel level MEMBER level)
  Q_PROPERTY(QString category MEMBER category)
  Q_PROPERTY(QString message MEMBER message)

public:
  LogLevel level = Debug;
  QString  category;
  QString  message;
};

class LogInterceptor : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  // TODO find a way not to repeat it !

  // using Level = LogLevel;
  enum Level
  {
    Debug,
    Info,
    Warning,
    Critical,
    Fatal,
  };
  Q_ENUM(Level)

  LogInterceptor(QObject* parent = nullptr);
  virtual ~LogInterceptor();

signals:
  void debug(LogEntry const& entry);
  void info(LogEntry const& entry);
  void warning(LogEntry const& entry);
  void critical(LogEntry const& entry);
  void fatal(LogEntry const& entry);

private:
  uint32_t               maxEntries_    = 10;
  QtMessageHandler       parentHandler_ = nullptr;
  static LogInterceptor* instance_;
  static void handler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};