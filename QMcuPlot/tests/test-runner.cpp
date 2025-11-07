#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QLoggingCategory>
#include <QQuickWindow>

#include <QExposeEvent>
#include <QVulkanInstance>

int main(int argc, char** argv)
{
  QLoggingCategory::setFilterRules("qt.scenegraph.general=true");

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

  QGuiApplication       app{argc, argv};
  QQmlApplicationEngine engine;
  engine.loadFromModule("QPlotTest", QML_MAIN);
  return app.exec();
}