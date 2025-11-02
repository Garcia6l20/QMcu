#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char** argv)
{
  QApplication app{argc, argv};

  QQmlApplicationEngine engine;
  engine.loadFromModule("QPlotTest", QML_MAIN);
  return app.exec();
}