#pragma once

#include <QObject>
#include <QQmlListProperty>
#include <QtQmlIntegration>

class VariableProxy;

class VariableProxyGroup : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  // Q_CLASSINFO("DefaultProperty", "proxies")

  Q_PROPERTY(QQmlListProperty<VariableProxy> proxies READ proxies NOTIFY proxiesChanged)

public:
  explicit VariableProxyGroup(QObject* parent = nullptr);

  QQmlListProperty<VariableProxy> proxies();

  void addProxy(VariableProxy* proxy);
  void removeProxy(VariableProxy* proxy);

public slots:
  void update();

signals:
  void proxiesChanged();

private:
  QList<VariableProxy*> proxies_;
};
