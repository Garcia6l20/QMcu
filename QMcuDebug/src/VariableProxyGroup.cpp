#include <QMcu/Debug/VariableProxy.hpp>
#include <QMcu/Debug/VariableProxyGroup.hpp>

VariableProxyGroup::VariableProxyGroup(QObject* parent) : QObject{parent}
{
}

void VariableProxyGroup::update()
{
  for(auto* p : proxies_)
  {
    p->update();
  }
}

void VariableProxyGroup::addProxy(VariableProxy* proxy)
{
  if(not proxies_.contains(proxy))
  {
    proxies_.append(proxy);
    proxy->setGroup(this);
    emit proxiesChanged();
  }
}
void VariableProxyGroup::removeProxy(VariableProxy* proxy)
{
  if(proxies_.contains(proxy))
  {
    proxies_.removeAll(proxy);
    proxy->setGroup(nullptr);
    emit proxiesChanged();
  }
}

QQmlListProperty<VariableProxy> VariableProxyGroup::proxies()
{
  return QQmlListProperty<VariableProxy>(
      this,
      this,
      [](QQmlListProperty<VariableProxy>* pl, VariableProxy* p)
      {
        auto* self = static_cast<VariableProxyGroup*>(pl->object);
        self->addProxy(p);
      },
      [](QQmlListProperty<VariableProxy>* pl)
      {
        auto* self = static_cast<VariableProxyGroup*>(pl->object);
        return self->proxies_.count();
      },
      [](QQmlListProperty<VariableProxy>* pl, qsizetype at)
      {
        auto* self = static_cast<VariableProxyGroup*>(pl->object);
        return self->proxies_.at(at);
      },
      [](QQmlListProperty<VariableProxy>* pl)
      {
        auto* self = static_cast<VariableProxyGroup*>(pl->object);
        for(auto* p : self->proxies_)
        {
          self->removeProxy(p);
        }
      });
}