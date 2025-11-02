#pragma once

#include <QApplication>

class Application : public QApplication {
public:
  using QApplication::QApplication;
  virtual ~Application() = default;

  Application &app() { return *static_cast<Application *>(qApp); }
};