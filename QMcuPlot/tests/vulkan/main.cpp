#include <QGuiApplication>
#include <QLoggingCategory>
#include <QtQuick/QQuickView>

int main(int argc, char** argv)
{
  QLoggingCategory::setFilterRules("qt.scenegraph.general=true");

  // Fix missing window header & borders on Linux Wayland
  // #ifdef OS_LINUX
  if(qgetenv("XDG_SESSION_TYPE") == "wayland")
  {
    qDebug("Force Wayland to use XCB");
    qputenv("QT_QPA_PLATFORM", "xcb");
  }
  // #endif

  QGuiApplication app(argc, argv);

  // This example needs Vulkan. It will not run otherwise.
  QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

  QQuickView view;
  view.setResizeMode(QQuickView::SizeRootObjectToView);
  view.setSource(QUrl("qrc:///scenegraph/vulkanunderqml/main.qml"));
  view.show();

  return app.exec();
}