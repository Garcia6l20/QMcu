#pragma once

#include <QMcu/Plot/GL/Types.hpp>

#include <QMatrix4x4>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QTransform>

#include <functional>

struct PlotContext
{
  using real_t                   = double;
  static constexpr auto   limits = std::numeric_limits<real_t>();
  static constexpr real_t nan    = limits.quiet_NaN();

  static QMatrix4x4 identity()
  {
    QMatrix4x4 m;
    m.setToIdentity();
    return m;
  };

  struct DataInfo
  {
    QMetaType::Type type;
    real_t          scaleMin = nan; // deprecated
    real_t          scaleMax = nan; // deprecated

    // transformation matrix: data-space to NDC space
    QMatrix4x4 toNdc   = identity();
    QMatrix4x4 fromNdc = identity();
  } data;

  struct UnitInfo
  {
    QString    unitName;
    double     multiplier = 1.0;
    double     offset     = 0.0;
    QMatrix4x4 toData     = identity();
    QMatrix4x4 fromData   = identity();
    QMatrix4x4 toNdc      = identity();
    QMatrix4x4 fromNdc    = identity();
    QMatrix4x4 ndcToData  = identity();
    QMatrix4x4 dataToNdc  = identity();
  } unit;

  struct ViewInfo
  {
    QSize      viewport;    // pixel size
    bool       autoScale = true;
    QMatrix4x4 transform = identity();

  } view;

  struct GLInfo
  {
    size_t stride    = 0; /// Stride between elements
    size_t elem_size = 0; /// Bytes of one element

    inline auto full_range() noexcept
    {
      return _range;
    }

    inline auto current_range() noexcept
    {
      return _current_range;
    }

    inline auto current_byte_count()
    {
      return _current_range.size_bytes();
    }

    inline auto current_sample_count()
    {
      return current_byte_count() / elem_size;
    }

    inline size_t current_byte_offset()
    {
      return _current_range.data() - _range.data();
    }

    inline auto current_sample_offset()
    {
      return current_byte_offset() / elem_size;
    }

    std::span<std::byte>  _range;
    std::span<std::byte>  _current_range;
    GLuint                _gl_handle = 0;
    GLuint                _gl_type;
    std::function<void()> _glVertexAttribPointer;
  } vbo;

  template <typename Fn> void visitType(Fn&& fn)
  {
    if(false)
    {
    }
#define X(__type, __qt_type, __gl_type)                           \
  else if(data.type == __qt_type)                                 \
  {                                                               \
    std::forward<decltype(fn)>(fn).template operator()<__type>(); \
  }
    QPLOT_BASIC_TYPE_MAP(X)
#undef X
    else
    {
      // std::abort(); // No op
    }
  }

  template <typename Fn> void visitCurrentData(Fn&& fn)
  {
    visitType(
        [&]<typename T>
        {
          const auto bytes = vbo.current_range();
          fn(std::span(reinterpret_cast<T*>(bytes.data()), bytes.size_bytes() / sizeof(T)));
        });
  }
  template <typename Fn> void visitFullData(Fn&& fn)
  {
    visitType(
        [&]<typename T>
        {
          const auto bytes = vbo.full_range();
          fn(std::span(reinterpret_cast<T*>(bytes.data()), bytes.size_bytes() / sizeof(T)));
        });
  }
};
