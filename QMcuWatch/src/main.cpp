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

QStringList
    patchPaths(QStringList const& expectedPaths, QString const& suffix, QStringList const originals)
{
  QStringList newPaths;
  newPaths.reserve(expectedPaths.size());
  for(auto const& ep : expectedPaths)
  {
    newPaths.append(ep + "/" + suffix);
  }
  for(auto const& op : originals)
  {
    if(not newPaths.contains(op))
    {
      newPaths.append(op);
    }
  }
  return newPaths;
}

int main(int argc, char** argv)
{
  qSetMessagePattern("[%{time mm:ss.zzz}][%{type}] %{category}: %{message}");

  QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

  // // Fix missing window header & borders on Linux Wayland
  // if(qgetenv("XDG_SESSION_TYPE") == "wayland")
  // {
  //   qDebug("Force Wayland to use XCB");
  //   qputenv("QT_QPA_PLATFORM", "xcb");
  // }

  // setup format for nice rendering hints
  qputenv("QSG_ANTIALIASING_METHOD",
          "msaa"); // NOTE: this avoids a long loading time while Qt tries to resolve this
  QSurfaceFormat fmt;
  fmt.setDepthBufferSize(24);
  fmt.setStencilBufferSize(8);
  fmt.setSamples(8);
  QSurfaceFormat::setDefaultFormat(fmt);

  const auto expectedBasePaths = QStringList()
#ifdef QMCU_FORCE_QML_DEV_PLUGINS
                              << QMCU_FORCE_QML_DEV_PLUGINS
#endif
                              << QDir::homePath()
                               + "/.local/lib/qt6"
                              << "/usr/local/lib/qt6"
                              << "/usr/lib/qt6";

  QCoreApplication::setLibraryPaths(
      patchPaths(expectedBasePaths, "plugins", QCoreApplication::libraryPaths()));

  QGuiApplication app(argc, argv);
  app.setOrganizationName("tpl");
  app.setOrganizationDomain("dev");
  app.setApplicationName("QMcuWatch");

  QCommandLineParser parser;
  QCommandLineOption configOpt{QStringList() << "c" << "config", "Configuration file", "config"};
  parser.addOption(configOpt);

  parser.addPositionalArgument("QMLFILE", "User's qml file to run");

  parser.process(app);

  if(parser.positionalArguments().empty())
  {
    qWarning() << parser.helpText();
    return -1;
  }

  QQmlApplicationEngine engine;
  engine.setImportPathList(patchPaths(expectedBasePaths, "qml", engine.importPathList()));

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
    if(not fin.open(QIODevice::ReadOnly))
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

  const auto scriptPath = parser.positionalArguments()[0];
  QFileInfo  info(scriptPath);
  QDir::setCurrent(info.absoluteDir().absolutePath());
  engine.load(info.fileName());

  return app.exec();
}