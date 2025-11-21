#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QLoggingCategory>
#include <QQuickWindow>

#include <QDir>
#include <QExposeEvent>
#include <QVulkanInstance>

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
  // QLoggingCategory::setFilterRules("qt.scenegraph.general=true");

  qSetMessagePattern("[%{time mm:ss.zzz}][%{type}] %{category}: %{message}");

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

  QLoggingCategory::setFilterRules(
      R"(qt.scenegraph.general=true
qt.qml.*.debug=true
qt.qml.overloadresolution.debug=false
qt.qml.delegatemodel.recycling=false
qt.qml.gc.*=false
)");

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

  QGuiApplication app{argc, argv};

  QVulkanInstance inst;

  inst.setLayers({"VK_LAYER_KHRONOS_validation"});
  inst.setApiVersion(QVersionNumber(1, 4));
  inst.setExtensions({
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
      VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME,
      VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME,
      "VK_KHR_multiview",
      "VK_KHR_maintenance2",
      "VK_KHR_create_renderpass2",
      "VK_KHR_get_physical_device_properties2",
      "VK_EXT_line_rasterization",
  });

  if(!inst.create())
  {
    qFatal("Failed to create Vulkan instance with validation layers");
  }

  QQmlApplicationEngine engine;

  engine.setImportPathList(patchPaths(expectedBasePaths, "qml", engine.importPathList()));

  engine.loadFromModule("QPlotTest", QML_MAIN);

  QQuickWindow* win = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
  Q_ASSERT(win);
  win->setVulkanInstance(&inst);

  return app.exec();
}