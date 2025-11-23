#include <QMcu/Utils/LogInterceptor.hpp>

LogInterceptor* LogInterceptor::instance_ = nullptr;

LogInterceptor::LogInterceptor(QObject* parent) : QObject(parent)
{
  if(instance_ != nullptr)
  {
    qFatal() << "LogInterceptor is singleton";
  }
  LogInterceptor::instance_ = this;

  parentHandler_ = qInstallMessageHandler(LogInterceptor::handler);
}

LogInterceptor::~LogInterceptor()
{
  LogInterceptor::instance_ = nullptr;
  qInstallMessageHandler(parentHandler_);
}

void LogInterceptor::handler(QtMsgType                 type,
                             const QMessageLogContext& context,
                             const QString&            message)
{
  if(LogInterceptor::instance_ == nullptr)
  {
    return;
  }

  auto& interceptor = *LogInterceptor::instance_;

  if(interceptor.parentHandler_)
  {
    interceptor.parentHandler_(type, context, message);
  }

  auto entry = LogEntry{};
  entry.category = context.category;
  entry.message  = message;
  switch(type)
  {
    case QtMsgType::QtInfoMsg:
      entry.level = LogLevel::Info;
      emit interceptor.info(entry);
      break;
    case QtMsgType::QtWarningMsg:
      entry.level = LogLevel::Warning;
      emit interceptor.warning(entry);
      break;
    case QtMsgType::QtCriticalMsg:
      entry.level = LogLevel::Critical;
      emit interceptor.critical(entry);
      break;
    case QtMsgType::QtFatalMsg:
      entry.level = LogLevel::Fatal;
      emit interceptor.fatal(entry);
      break;
    default:
    case QtMsgType::QtDebugMsg:
      entry.level = LogLevel::Debug;
      emit interceptor.debug(entry);
      break;
  }
}
