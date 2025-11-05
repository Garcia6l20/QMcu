#include <QMcu/Debug/Debugger.hpp>
#include <QMcu/Debug/StLinkProbe.hpp>
#include <QMcu/Debug/Type.hpp>
#include <QMcu/Debug/Variable.hpp>

#include <Logging.hpp>

#include <magic_enum/magic_enum.hpp>

using namespace lldb;

Variable::Variable(SBValue value, Debugger* parent) : QObject(parent), value_(value)
{
  cache_.detach();

  if(StLinkProbe::instance() != nullptr)
  {
    resolve(); // direct resolution possible
  }
  else
  {
    loadValue_ = [](QVariant& out) { return false; };
    connect(Debugger::instance(),
            &Debugger::launchedChanged,
            this,
            [this](bool launched)
            {
              if(launched)
              {
                resolve();
              }
            });
  }
}

void Variable::resolve()
{
  using reader_fn       = std::function<bool(std::span<std::byte>)>;
  const auto get_reader = [dbg = debugger()](uint64_t address) -> reader_fn
  {
    if(auto* sl = StLinkProbe::instance(); sl != nullptr)
    {
      return [sl, address](std::span<std::byte> data)
      {
        // qDebug(lcWatcher) << "reading" << address;
        return sl->read(address, data);
      };
    }
    else
    {
      return [sl, address, dbg](std::span<std::byte> data)
      {
        // qDebug(lcWatcher) << "reading" << address;
        lldb::SBError error;
        dbg->process().ReadMemory(address, data.data(), data.size_bytes(), error);
        if(error.Fail())
        {
          qCritical(lcWatcher) << "Failed to read memory at address" << address << ":"
                               << error.GetCString();
          return false;
        }
        return true;
      };
    }
  };

  const auto get_address = [dbg = debugger()](lldb::SBValue& value)
  {
    if(StLinkProbe::instance())
    {
      return value.GetAddress().GetFileAddress();
    }
    else
    {
      return value.GetAddress().GetLoadAddress(dbg->target());
    }
  };

  const auto get_reader_for =
      [dbg = debugger(), &get_reader, &get_address]<typename T>(lldb::SBValue& value)
  {
    const auto address = get_address(value);
    return [reader = get_reader(address)](void* output)
    { return reader(std::span<std::byte>{reinterpret_cast<std::byte*>(output), sizeof(T)}); };
  };

  const auto get_meta_type = [](lldb::SBValue& value)
  {
    auto type  = value.GetType();
    auto ctype = type.GetCanonicalType();
    qDebug(lcWatcher) << "query canonical loader for" << type.GetName();
    const auto btype = ctype.GetBasicType();
    assert(btype != lldb::eBasicTypeInvalid);
    const auto size = type.GetByteSize();
    switch(btype)
    {
      case lldb::eBasicTypeBool:
        assert(sizeof(bool) == size);
        return QMetaType::Bool;
      case lldb::eBasicTypeChar:
      case lldb::eBasicTypeChar8:
      case lldb::eBasicTypeSignedChar:
        assert(sizeof(int8_t) == size);
        return QMetaType::Char;
      case lldb::eBasicTypeChar16:
      case lldb::eBasicTypeSignedWChar:
      case lldb::eBasicTypeShort:
      case lldb::eBasicTypeHalf:
        assert(sizeof(int16_t) == size);
        return QMetaType::Short;
      case lldb::eBasicTypeInt:
      case lldb::eBasicTypeLong:
        assert(sizeof(int32_t) == size);
        return QMetaType::Int;
      case lldb::eBasicTypeLongLong:
        assert(sizeof(int64_t) == size);
        return QMetaType::LongLong;
      case lldb::eBasicTypeUnsignedChar:
        assert(sizeof(uint8_t) == size);
        return QMetaType::UChar;
      case lldb::eBasicTypeUnsignedShort:
        assert(sizeof(uint16_t) == size);
        return QMetaType::UShort;
      case lldb::eBasicTypeUnsignedInt:
      case lldb::eBasicTypeUnsignedLong:
        assert(sizeof(uint32_t) == size);
        return QMetaType::UInt;
      case lldb::eBasicTypeUnsignedLongLong:
        assert(sizeof(uint64_t) == size);
        return QMetaType::ULongLong;
      case lldb::eBasicTypeFloat:
        assert(sizeof(float) == size);
        return QMetaType::Float;
      case lldb::eBasicTypeDouble:
        assert(sizeof(double) == size);
        return QMetaType::Double;
      default:
        return QMetaType::Void;
    }
  };

  using var_reader_fn = std::function<bool(QVariant&)>;
  const auto get_canonical_loader =
      [this, &get_reader_for, &get_meta_type](lldb::SBValue& value) -> var_reader_fn
  {
    switch(get_meta_type(value))
    {
      case QMetaType::Bool:
        return [this, reader = get_reader_for.template operator()<bool>(value_)](QVariant& var) {
          if(var.metaType().id() != QMetaType::Bool)
          {
            var.emplace<bool>();
          }
          return reader(var.data());
        };
        break;
      case QMetaType::Char:
        return [this, reader = get_reader_for.template operator()<int8_t>(value_)](QVariant& var) {
          if(var.metaType().id() != QMetaType::Char)
          {
            var.emplace<int8_t>();
          }
          return reader(var.data());
        };
        break;
      case QMetaType::Short:
        return [this, reader = get_reader_for.template operator()<int16_t>(value_)](QVariant& var) {
          if(var.metaType().id() != QMetaType::Short)
          {
            var.emplace<int16_t>();
          }
          return reader(var.data());
        };
        break;
      case QMetaType::Int:
        return [this, reader = get_reader_for.template operator()<int32_t>(value_)](QVariant& var) {
          if(var.metaType().id() != QMetaType::Int)
          {
            var.emplace<int32_t>();
          }
          return reader(var.data());
        };
        break;
      case QMetaType::LongLong:
        return [this, reader = get_reader_for.template operator()<int64_t>(value_)](QVariant& var) {
          if(var.metaType().id() != QMetaType::LongLong)
          {
            var.emplace<int64_t>();
          }
          return reader(var.data());
        };
        break;
      case QMetaType::UChar:
        return [this, reader = get_reader_for.template operator()<uint8_t>(value_)](QVariant& var) {
          if(var.metaType().id() != QMetaType::UChar)
          {
            var.emplace<uint8_t>();
          }
          return reader(var.data());
        };
        break;
      case QMetaType::UShort:
        return
            [this, reader = get_reader_for.template operator()<uint16_t>(value_)](QVariant& var) {
              if(var.metaType().id() != QMetaType::UShort)
              {
                var.emplace<uint16_t>();
              }
              return reader(var.data());
            };
        break;
      case QMetaType::UInt:
        return
            [this, reader = get_reader_for.template operator()<uint32_t>(value_)](QVariant& var) {
              if(var.metaType().id() != QMetaType::UInt)
              {
                var.emplace<uint32_t>();
              }
              return reader(var.data());
            };
        break;
      case QMetaType::ULongLong:
        return
            [this, reader = get_reader_for.template operator()<uint64_t>(value_)](QVariant& var) {
              if(var.metaType().id() != QMetaType::ULongLong)
              {
                var.emplace<uint64_t>();
              }
              return reader(var.data());
            };
        break;
      case QMetaType::Float:
        return [this, reader = get_reader_for.template operator()<float>(value_)](QVariant& var) {
          if(var.metaType().id() != QMetaType::Float)
          {
            var.emplace<float>();
          }
          return reader(var.data());
        };
        break;
      case QMetaType::Double:
        return [this, reader = get_reader_for.template operator()<double>(value_)](QVariant& var) {
          if(var.metaType().id() != QMetaType::Double)
          {
            var.emplace<double>();
          }
          return reader(var.data());
        };
        break;
      default:
        qFatal(lcWatcher) << "unhandled type for " << value.GetName();
        return [](QVariant& var) { return false; };
    }
  };

  const auto ctype = type()->canonicalBasicType();
  if(ctype != lldb::eBasicTypeInvalid)
  {
    loadValue_ = get_canonical_loader(value_);
  }
  else if(type()->isArray())
  {
    QList<int> extents;
    auto       tmp_val = value_;
    while(true)
    {
      const auto child_count = tmp_val.GetNumChildren();
      if(child_count == 0)
      {
        break;
      }
      extents.append(child_count);
      tmp_val = tmp_val.GetChildAtIndex(0);
    }

    if(extents.size() > 1)
    {
      qFatal(lcWatcher) << "Multidimensional arrays not handled yet";
    }

    // const auto address      = get_address(tmp_val);
    // auto       value_loader = get_canonical_loader(tmp_val);

    const size_t dimension_size = extents[0];

    const auto tid       = get_meta_type(tmp_val);
    const auto address   = get_address(value_);
    const auto valuesize = tmp_val.GetByteSize();
    auto       reader    = get_reader(address);
    void*      local_data;
    void*      cache_data;
    switch(tid)
    {
      case QMetaType::Bool:
      {
        // TODO: check for vector-of-bool potential problems
        using VecT = QVector<bool>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      case QMetaType::Char:
      {
        using VecT = QVector<int8_t>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      case QMetaType::UChar:
      {
        using VecT = QVector<uint8_t>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      case QMetaType::Short:
      {
        using VecT = QVector<int16_t>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      case QMetaType::UShort:
      {
        using VecT = QVector<uint16_t>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      case QMetaType::Int:
      {
        using VecT = QVector<int32_t>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      case QMetaType::UInt:
      {
        using VecT = QVector<uint32_t>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      case QMetaType::LongLong:
      {
        using VecT = QVector<int64_t>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      case QMetaType::ULongLong:
      {
        using VecT = QVector<uint64_t>;

        auto& local_v = local_.emplace<VecT>();
        local_v.resize(dimension_size);

        auto& cache_v = cache_.emplace<VecT>();
        cache_v.resize(dimension_size);

        local_data = local_v.data();
        cache_data = cache_v.data();
      }
      break;
      default:
        qFatal(lcWatcher) << "Unhandled array type" << type()->name();
    }

    const auto s0 = std::span{reinterpret_cast<std::byte*>(local_data), dimension_size * valuesize};
    const auto s1 = std::span{reinterpret_cast<std::byte*>(cache_data), dimension_size * valuesize};

    // load cache
    // reader(std::span{reinterpret_cast<std::byte*>(cache_data), dimension_size * valuesize});
    reader(s0);
    std::ranges::copy(s0, s1.begin());

    loadValue_ = [this,
                  dimension_size,
                  valuesize,
                  &var0  = local_,
                  &var1  = cache_,
                  s0     = s0,
                  s1     = s1,
                  reader = std::move(reader)](QVariant& out) mutable -> bool
    {
      const bool ok = reader(s1);
      if(ok)
      {
        if(not std::ranges::equal(s0, s1))
        {
          // swap local/cache data
          std::swap(var0, var1);
          std::swap(s0, s1);

          out = var0;
        }
      }
      return ok;
    };
  }
  else
  {
    qFatal(lcWatcher) << "unhandled type: " << type()->name();
  }

  qDebug(lcWatcher) << value_.GetName() << "resolved";
}

Debugger* Variable::debugger()
{
  return static_cast<Debugger*>(parent());
}

QString Variable::name()
{
  return value_.GetName();
}

uint64_t Variable::address()
{
  if(StLinkProbe::instance())
  {
    return value_.GetAddress().GetFileAddress();
  }
  else
  {
    auto dbg = debugger();
    return value_.GetAddress().GetLoadAddress(dbg->target());
  }
}

QString Variable::display()
{
  return QString("%1 %2").arg(type()->name(), name());
}

Type* Variable::type()
{
  auto* type = findChild<Type*>();
  if(type)
  {
    return type;
  }
  return new Type(value_.GetType(), this);
}

bool Variable::read(std::span<std::byte> data)
{
  auto const address = this->address();
  if(auto* sl = StLinkProbe::instance(); sl != nullptr)
  {
    return sl->read(address, data);
  }
  else
  {
    lldb::SBError error;
    debugger()->process().ReadMemory(address, data.data(), data.size_bytes(), error);
    if(error.Fail())
    {
      qCritical(lcWatcher) << "Failed to read memory at address" << address << ":"
                           << error.GetCString();
      return false;
    }
    return true;
  }
}

qreal Variable::readAsReal()
{
  qreal value = 0.0;

  const auto ctype = type()->canonicalBasicType();

  switch(ctype)
  {
    case lldb::eBasicTypeBool:
      value = read<bool>().value_or(false) ? 1 : 0;
      break;
    case lldb::eBasicTypeChar:
    case lldb::eBasicTypeChar8:
    case lldb::eBasicTypeSignedChar:
      value = read<int8_t>().value_or(0);
      break;
    case lldb::eBasicTypeChar16:
    case lldb::eBasicTypeSignedWChar:
    case lldb::eBasicTypeShort:
    case lldb::eBasicTypeHalf:
      value = read<int16_t>().value_or(0);
      break;
    case lldb::eBasicTypeInt:
    case lldb::eBasicTypeLong:
      value = read<int32_t>().value_or(0);
      break;
    case lldb::eBasicTypeLongLong:
      value = read<int64_t>().value_or(0);
      break;
    case lldb::eBasicTypeUnsignedChar:
      value = read<uint8_t>().value_or(0);
      break;
    case lldb::eBasicTypeUnsignedShort:
      value = read<uint16_t>().value_or(0);
      break;
    case lldb::eBasicTypeUnsignedInt:
    case lldb::eBasicTypeUnsignedLong:
      value = read<uint32_t>().value_or(0);
      break;
    case lldb::eBasicTypeUnsignedLongLong:
      value = read<uint64_t>().value_or(0);
      break;
    case lldb::eBasicTypeFloat:
      value = read<float>().value_or(0);
      break;
    case lldb::eBasicTypeDouble:
      value = read<double>().value_or(0);
      break;
    default:
      qFatal(lcWatcher) << "unhandled type: " << magic_enum::enum_name(ctype);
  }
  return value;
}
