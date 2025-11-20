#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QLoggingCategory>
#include <QQuickWindow>

#include <QExposeEvent>
#include <QVulkanInstance>

int main(int argc, char** argv)
{
  // QLoggingCategory::setFilterRules("qt.scenegraph.general=true");

  qSetMessagePattern("[%{time mm:ss.zzz}][%{type}] %{category}: %{message}");

  QLoggingCategory::setFilterRules("qt.scenegraph.general=true\n"
                                   "qt.qml.*.debug=true\n"
                                   "qt.qml.overloadresolution.debug=false");

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
  });

  if(!inst.create())
  {
    qFatal("Failed to create Vulkan instance with validation layers");
  }

  QQmlApplicationEngine engine;
  engine.loadFromModule("QPlotTest", QML_MAIN);

  QQuickWindow* win = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
  Q_ASSERT(win);
  win->setVulkanInstance(&inst);

  return app.exec();
}