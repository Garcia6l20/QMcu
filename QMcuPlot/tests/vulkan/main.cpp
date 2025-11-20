#include <QGuiApplication>
#include <QLoggingCategory>
#include <QtQuick/QQuickView>

#include <QVulkanInstance>

int main(int argc, char** argv)
{
  QLoggingCategory::setFilterRules("qt.scenegraph.general=true");

  // Fix missing window header & borders on Linux Wayland
  // #ifdef OS_LINUX
  // if(qgetenv("XDG_SESSION_TYPE") == "wayland")
  // {
  //   qDebug("Force Wayland to use XCB");
  //   qputenv("QT_QPA_PLATFORM", "xcb");
  // }
  // #endif

  // setup format for nice rendering hints
  qputenv("QSG_ANTIALIASING_METHOD",
          "msaa"); // NOTE: this avoids a long loading time while Qt tries to resolve this
  QSurfaceFormat fmt;
  fmt.setDepthBufferSize(24);
  fmt.setStencilBufferSize(8);
  fmt.setSamples(8);
  QSurfaceFormat::setDefaultFormat(fmt);

  QGuiApplication app(argc, argv);

  QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

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

  // This example needs Vulkan. It will not run otherwise.
  QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

  QQuickView view;
  view.setVulkanInstance(&inst);
  view.setResizeMode(QQuickView::SizeRootObjectToView);
  view.setSource(QUrl("qrc:///scenegraph/vulkanunderqml/main.qml"));
  view.show();

  return app.exec();
}