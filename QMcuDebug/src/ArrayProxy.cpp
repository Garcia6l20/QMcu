#include <QMcu/Debug/ArrayProxy.hpp>

#include <Logging.hpp>

ArrayProxy::ArrayProxy(QObject* parent) : VariableProxy{parent}
{
  connect(this,
          &VariableProxy::variableResolved,
          this,
          [this]
          {
            auto type = variable()->type();
            if(not type->isArray())
            {
              qCritical(lcDebugger) << "Variable is not an array:" << variable()->name();
              return;
            }
            auto const& extents = variable()->type()->extents();
            Q_ASSERT(not extents.isEmpty());
            size_ = extents[0];
            emit sizeChanged();
          });
}
