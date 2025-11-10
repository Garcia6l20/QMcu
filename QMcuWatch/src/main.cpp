#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QJsonDocument>
#include <QJsonObject>

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <QFile>

#include <QDebug>
#include <QDir>
#include <QQuickWindow>
#include <QSurfaceFormat>

int main(int argc, char** argv)
{
  QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

  // Fix missing window header & borders on Linux Wayland
  if(qgetenv("XDG_SESSION_TYPE") == "wayland")
  {
    qDebug("Force Wayland to use XCB");
    qputenv("QT_QPA_PLATFORM", "xcb");
  }

  // setup format for nice rendering hints
  qputenv("QSG_ANTIALIASING_METHOD",
          "msaa"); // NOTE: this avoids a long loading time while Qt tries to resolve this
  QSurfaceFormat fmt;
  fmt.setDepthBufferSize(24);
  fmt.setStencilBufferSize(8);
  fmt.setSamples(8);
  QSurfaceFormat::setDefaultFormat(fmt);

  {
    QStringList paths = QCoreApplication::libraryPaths();

    qDebug() << "Initial plugin paths:" << paths;

    for(auto const& p : QStringList()
                            << "/usr/lib/qt6/plugins"
                            << "/usr/local/lib/qt6/plugins"
                            << QDir::homePath()
                            + "/.local/lib/qt6/plugins")
    {
      if(/* QDir(p).exists() and */ not paths.contains(p))
      {
        paths.append(p);
      }
    }

    QCoreApplication::setLibraryPaths(paths);

    qDebug() << "Using plugin paths:" << paths;
  }

  qSetMessagePattern("[%{time mm:ss.zzz}][%{type}] %{category}: %{message}");
  // qSetMessagePattern("[%{time mm:ss.zzz}][%{type}] %{category}: %{message} (%{file}:%{line})");

  QGuiApplication app(argc, argv);
  app.setOrganizationName("tpl");
  app.setOrganizationDomain("dev");
  app.setApplicationName("QMcuWatch");

  QCommandLineParser parser;
  QCommandLineOption configOpt{QStringList() << "c" << "config", "Configuration file", "config"};
  parser.addOption(configOpt);

  parser.addPositionalArgument("QMLFILE", "User's qml file to run");

  parser.process(app);

  QQmlApplicationEngine engine;
#ifndef FORCE_DEVELOPMENT_QML_DIR
  engine.addImportPath("/usr/lib/qt6/qml");
  engine.addImportPath("/usr/local/lib/qt6/qml");
  engine.addImportPath(QDir::homePath() + "/.local/lib/qt6/qml");
#else
  engine.addImportPath(FORCE_DEVELOPMENT_QML_DIR);
#endif

  QObject::connect(
      &engine,
      &QQmlApplicationEngine::objectCreationFailed,
      &app,
      []() { QCoreApplication::exit(-1); },
      Qt::QueuedConnection);

  auto& ctx = *engine.rootContext();

  QJsonDocument config;

  if(parser.isSet(configOpt))
  {
    QFile fin{parser.value(configOpt)};
    fin.open(QIODevice::ReadOnly);
    if(not fin.isOpen())
    {
      qFatal() << "Fail to open configuration:" << fin.errorString();
    }
    QJsonParseError parseError;
    config = QJsonDocument::fromJson(fin.readAll(), &parseError);
    if(parseError.error != QJsonParseError::NoError)
    {
      qFatal() << "Fail to read configuration:" << parseError.errorString();
    }
    ctx.setContextProperty("config", config.object());
  }

  engine.load(parser.positionalArguments()[0]);

  return app.exec();
}