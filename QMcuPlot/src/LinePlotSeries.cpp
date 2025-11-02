#include <QMcu/Plot/LinePlotSeries.hpp>
#include <QOpenGLShaderProgram>

#include <QMcu/Plot/GL/IncludeAPI.hpp>
#include <Logging.hpp>

#include <fmt/format.h>

static const QColor defaultColors[] = {
    QColorConstants::Svg::cyan,
    QColorConstants::Svg::magenta,
    QColorConstants::Svg::coral,
    QColorConstants::Svg::fuchsia,
    QColorConstants::Svg::crimson,
    QColorConstants::Svg::aliceblue,
    QColorConstants::Svg::chartreuse,
    QColorConstants::Svg::aqua,
    QColorConstants::Svg::beige,
    QColorConstants::Svg::bisque,
    QColorConstants::Svg::blanchedalmond,
    QColorConstants::Svg::blueviolet,
    QColorConstants::Svg::brown,
    QColorConstants::Svg::burlywood,
    QColorConstants::Svg::cadetblue,
    QColorConstants::Svg::chocolate,
    QColorConstants::Svg::cornflowerblue,
    QColorConstants::Svg::antiquewhite,
    QColorConstants::Svg::cornsilk,
    QColorConstants::Svg::deeppink,
    QColorConstants::Svg::deepskyblue,
    QColorConstants::Svg::dimgray,
    QColorConstants::Svg::dimgrey,
    QColorConstants::Svg::dodgerblue,
    QColorConstants::Svg::firebrick,
    QColorConstants::Svg::floralwhite,
    QColorConstants::Svg::forestgreen,
    QColorConstants::Svg::gainsboro,
    QColorConstants::Svg::ghostwhite,
    QColorConstants::Svg::gold,
    QColorConstants::Svg::goldenrod,
    QColorConstants::Svg::aquamarine,
    QColorConstants::Svg::green,
    QColorConstants::Svg::blue,
    QColorConstants::Svg::greenyellow,
    QColorConstants::Svg::honeydew,
    QColorConstants::Svg::hotpink,
    QColorConstants::Svg::indianred,
    QColorConstants::Svg::indigo,
    QColorConstants::Svg::ivory,
    QColorConstants::Svg::khaki,
    QColorConstants::Svg::lavender,
    QColorConstants::Svg::lavenderblush,
    QColorConstants::Svg::lawngreen,
    QColorConstants::Svg::lemonchiffon,
    QColorConstants::Svg::lime,
    QColorConstants::Svg::limegreen,
    QColorConstants::Svg::linen,
    QColorConstants::Svg::maroon,
    QColorConstants::Svg::midnightblue,
    QColorConstants::Svg::mintcream,
    QColorConstants::Svg::mistyrose,
    QColorConstants::Svg::moccasin,
    QColorConstants::Svg::navajowhite,
    QColorConstants::Svg::azure,
    QColorConstants::Svg::navy,
    QColorConstants::Svg::oldlace,
    QColorConstants::Svg::olive,
    QColorConstants::Svg::olivedrab,
    QColorConstants::Svg::orange,
    QColorConstants::Svg::orangered,
    QColorConstants::Svg::orchid,
    QColorConstants::Svg::palegoldenrod,
    QColorConstants::Svg::palegreen,
    QColorConstants::Svg::paleturquoise,
    QColorConstants::Svg::palevioletred,
    QColorConstants::Svg::papayawhip,
    QColorConstants::Svg::peachpuff,
    QColorConstants::Svg::peru,
    QColorConstants::Svg::pink,
    QColorConstants::Svg::plum,
    QColorConstants::Svg::powderblue,
    QColorConstants::Svg::purple,
    QColorConstants::Svg::red,
    QColorConstants::Svg::rosybrown,
    QColorConstants::Svg::royalblue,
    QColorConstants::Svg::saddlebrown,
    QColorConstants::Svg::salmon,
    QColorConstants::Svg::sandybrown,
    QColorConstants::Svg::seagreen,
    QColorConstants::Svg::seashell,
    QColorConstants::Svg::sienna,
    QColorConstants::Svg::silver,
    QColorConstants::Svg::skyblue,
    QColorConstants::Svg::slateblue,
    QColorConstants::Svg::slategray,
    QColorConstants::Svg::slategrey,
    QColorConstants::Svg::snow,
    QColorConstants::Svg::springgreen,
    QColorConstants::Svg::steelblue,
    QColorConstants::Svg::tan,
    QColorConstants::Svg::teal,
    QColorConstants::Svg::thistle,
    QColorConstants::Svg::tomato,
    QColorConstants::Svg::turquoise,
    QColorConstants::Svg::violet,
    QColorConstants::Svg::wheat,
    QColorConstants::Svg::whitesmoke,
    QColorConstants::Svg::yellow,
    QColorConstants::Svg::yellowgreen,
};

LinePlotSeries::LinePlotSeries(QObject* parent)
    : AbstractPlotSeries(parent),
      lineColor_(defaultColors[id() % (sizeof(defaultColors) / sizeof(defaultColors[0]))])
{
}

LinePlotSeries::~LinePlotSeries()
{
  if(ctx_.vbo._gl_handle)
  {
    glDeleteBuffers(1, &ctx_.vbo._gl_handle);
  }
}

template <typename T> constexpr T bitmask(unsigned from, unsigned to)
{
  return T(((1u << (to - from + 1)) - 1u) << from);
}

static constexpr std::string_view decoders = R"GLSL(
// 8-bit unsigned
uint decodeU8(uint byteIndex) {
    const uint wordIndex = byteIndex / 4u;
    const uint byteInWord = byteIndex % 4u;
    return (data[wordIndex] >> (byteInWord * 8u)) & 0xFFu;
}

// 8-bit signed
int decodeI8(uint byteIndex) {
    const uint uval = decodeU8(byteIndex);
    return (int(uval) << 8) >> 8; // sign-extend
}

// 16-bit unsigned (little-endian)
uint decodeU16(uint byteIndex) {
    const uint b0 = decodeU8(byteIndex + 0u);
    const uint b1 = decodeU8(byteIndex + 1u);
    return b0 | (b1 << 8u);
}

// 16-bit signed (little-endian)
int decodeI16(uint byteIndex) {
    const uint uval = decodeU16(byteIndex);
    return (int(uval) << 16) >> 16; // sign-extend
}

// 32-bit unsigned (little-endian)
uint decodeU32(uint byteIndex) {
    const uint b0 = decodeU8(byteIndex + 0u);
    const uint b1 = decodeU8(byteIndex + 1u);
    const uint b2 = decodeU8(byteIndex + 2u);
    const uint b3 = decodeU8(byteIndex + 3u);
    return b0 | (b1 << 8u) | (b2 << 16u) | (b3 << 24u);
}

// 32-bit signed (little-endian)
int decodeI32(uint byteIndex) {
    return int(decodeU32(byteIndex));
}
)GLSL";

void registerIncludes(glsl::IncludeAPI const& api)
{
  static constexpr const std::string_view virtualPath = "/common/decoders.glsl";
  api.registerInclude("/common/decoders.glsl", decoders);
}

template <typename T> constexpr auto decoderFunction()
{
  static constexpr size_t n_bits = (8 * sizeof(T));
  return fmt::format("const float rawY = decodeI{bitCount}(byteIndex);",
                     fmt::arg("encoderType", std::is_signed_v<T> ? 'I' : 'U'),
                     fmt::arg("bitCount", n_bits));
};

std::string LinePlotSeries::generateShaderSource(PlotContext& ctx)
{
  std::string      typeStr;
  std::string      readExpr;
  std::string_view required_decoders = "";

  const auto make_integer_binding = [&]<std::integral T>(std::type_identity<T>)
  {
    constexpr auto limits = std::numeric_limits<T>();

    ctx.data.scaleMin = limits.min();
    ctx.data.scaleMax = limits.max();

    const double min = double(limits.min());
    const double max = double(limits.max());

    ctx.data.fromNdc.setToIdentity();
    ctx.data.fromNdc.viewport(0.0, min, 1.0, max - min);
    ctx.data.toNdc = ctx.data.fromNdc.inverted();

    if constexpr(std::is_unsigned_v<T>)
    {
      typeStr = "uint";
    }
    else
    {
      typeStr = "int";
    }

    readExpr = decoderFunction<T>();

    if constexpr(std::same_as<T, uint32_t>)
    {
      ctx_.vbo._gl_type = GL_UNSIGNED_INT;
    }
    else if constexpr(std::same_as<T, int32_t>)
    {
      ctx_.vbo._gl_type = GL_INT;
    }
    else if constexpr(std::same_as<T, uint16_t>)
    {
      ctx_.vbo._gl_type = GL_UNSIGNED_SHORT;
    }
    else if constexpr(std::same_as<T, int16_t>)
    {
      ctx_.vbo._gl_type = GL_SHORT;
    }
    else if constexpr(std::same_as<T, uint8_t>)
    {
      ctx_.vbo._gl_type = GL_UNSIGNED_BYTE;
    }
    else if constexpr(std::same_as<T, int8_t>)
    {
      ctx_.vbo._gl_type = GL_BYTE;
    }
    else
    {
      static_assert(always_false_c<T>, "unhandled type");
    }

    ctx.vbo._glVertexAttribPointer = [this]
    { glVertexAttribIPointer(0, 1, ctx_.vbo._gl_type, ctx_.vbo.stride, nullptr); };

    if(ctx.vbo.stride == 0)
    {
      ctx.vbo.stride = sizeof(T);
    }
    ctx.vbo.elem_size = sizeof(T);
    required_decoders = decoders;
  };

  switch(ctx.data.type)
  {
    case QMetaType::Float:
      typeStr  = "float";
      readExpr = "const float rawY = data[byteIndex / 4];";
      if(ctx.vbo.stride == 0)
      {
        ctx.vbo.stride = sizeof(float);
      }
      ctx.vbo.elem_size              = sizeof(float);
      ctx.vbo._gl_type               = GL_FLOAT;
      ctx.vbo._glVertexAttribPointer = [this]
      { glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, ctx_.vbo.stride, nullptr); };
      break;
    case QMetaType::Double:
      typeStr  = "double";
      readExpr = "const double rawY = data[byteIndex / 8];";
      if(ctx.vbo.stride == 0)
      {
        ctx.vbo.stride = sizeof(double);
      }
      ctx.vbo.elem_size              = sizeof(double);
      ctx.vbo._gl_type               = GL_DOUBLE;
      ctx.vbo._glVertexAttribPointer = [this]
      { glVertexAttribPointer(0, 1, GL_DOUBLE, GL_FALSE, ctx_.vbo.stride, nullptr); };
      break;
    case QMetaType::Int:
    {
      make_integer_binding(std::type_identity<int32_t>());
    }
    break;
      break;
    case QMetaType::UInt:
    {
      make_integer_binding(std::type_identity<uint32_t>());
    }
    break;
    case QMetaType::Short:
    {
      make_integer_binding(std::type_identity<int16_t>());
    }
    break;
    case QMetaType::UShort:
    {
      make_integer_binding(std::type_identity<uint16_t>());
    }
    break;
    case QMetaType::Char:
    {
      make_integer_binding(std::type_identity<int8_t>());
    }
    break;
    case QMetaType::UChar:
    {
      make_integer_binding(std::type_identity<uint8_t>());
    }
    break;
    default:
      qFatal() << "Unhandled type";
  }

  return fmt::format(R"GLSL(#version 450 core

layout(std430, binding = 0) buffer InputData {{
    {type} data[];
}};

{decoders}

uniform mat4 u_dataToNdc;       // data -> NDC
uniform mat4 u_viewTransform;   // zoom & pan in NDC space

uniform uint u_byteCount;     // byte count
uniform uint u_byteOffset;    // byte offset
uniform uint u_sampleStride;  // sample stride

out vec2 vPosNdc;

void main() {{
    const uint byteIndex = u_byteOffset + uint(gl_VertexID) * u_sampleStride;
    {readExpr} // generated read expression

    const vec4 raw = vec4(float(gl_VertexID), rawY, 0.0, 1.0);
    const vec4 ndc = u_dataToNdc * raw;

    // Apply zoom/pan
    const vec4 view = u_viewTransform * ndc;

    gl_Position = view;

    vPosNdc = view.xy;

}})GLSL",
                     fmt::arg("type", typeStr),
                     fmt::arg("readExpr", readExpr),
                     fmt::arg("decoders", required_decoders));
}

bool LinePlotSeries::allocateGL(QSize const& viewport)
{
  ctx_.view.viewport = viewport;

  initializeOpenGLFunctions();

  // glEnable(GL_DEBUG_OUTPUT);
  // glDebugMessageCallback([](GLenum        source,
  //                           GLenum        type,
  //                           GLuint        id,
  //                           GLenum        severity,
  //                           GLsizei       length,
  //                           const GLchar* message,
  //                           const void*   userParam) { qDebug() << "GL debug:" << message; },
  //                        nullptr);

  auto* const provider = dataProvider();
  if(not provider)
  {
    return false;
  }

  if(not initializeDataProvider())
  {
    return false;
  }

  ctx_.vbo._current_range = ctx_.vbo.full_range();

  if(!ctx_.vbo._gl_handle)
  {
    qFatal(lcPlot).noquote()
        << provider->metaObject()->className()
        << "::initializePlotContext: must create a buffer (ie.: with createMappedBuffer<T>(N))";
  }

  const std::string vertexSrc = generateShaderSource(ctx_);
  // qDebug(lcPlot).noquote() << "Vertex shader:" << vertexSrc;

  program_ = std::make_unique<QOpenGLShaderProgram>();

  // 1 - Vertex: Extract data and transform it to NDC space
  if(!program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSrc.c_str()))
  {
    qFatal(lcPlot).noquote() << "Vertex shader failed to compile:\n" << program_->log();
  }

  if(lineStyle_ == LineStyle::Halo)
  {
    // 2 - Geometry: Create quads around every line pairs
    if(!program_->addShaderFromSourceCode(QOpenGLShader::Geometry,
                                          R"GLSL(
#version 450 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float u_thickness; // NDC thickness
out vec2 vLineUV;

void main()
{
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;

    // direction of the line
    vec2 dir = normalize(p1.xy - p0.xy);

    // perpendicular vector for thickness
    vec2 normal = vec2(-dir.y, dir.x) * u_thickness * 2.0;

    vec2 n0 = p0.xy - normal;
    vec2 n1 = p0.xy + normal;
    vec2 n2 = p1.xy - normal;
    vec2 n3 = p1.xy + normal;

    // emit 4 vertices (two triangles)
    vLineUV = vec2(0.0, -1.0);
    gl_Position = vec4(n0, 0.0, 1.0);
    EmitVertex();

    vLineUV = vec2(0.0, 1.0);
    gl_Position = vec4(n1, 0.0, 1.0);
    EmitVertex();

    vLineUV = vec2(1.0, -1.0);
    gl_Position = vec4(n2, 0.0, 1.0);
    EmitVertex();

    vLineUV = vec2(1.0, 1.0);
    gl_Position = vec4(n3, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
)GLSL"))
    {
      qFatal(lcPlot).noquote() << "Geometry shader failed to compile:\n" << program_->log();
    }

    // 3 - Fragment: apply smoothing based on the distance from line center
    if(!program_->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                          R"GLSL(
#version 450 core

in vec2 vLineUV;

uniform vec4 u_color;    // base color
uniform float u_thickness = 0.02;
uniform float u_glow = 2.0;

out vec4 fragColor;

void main() {
    float dist = abs(vLineUV.y); // distance from line center

    float core = smoothstep(u_thickness, 0.0, dist);
    float halo = exp(-pow(dist / u_thickness, 2.0) * 5.0) * u_glow;

    float intensity = core + halo;
    fragColor = vec4(u_color.rgb * intensity, clamp(intensity, 0.0, 1.0));
}
)GLSL"))
    {
      qFatal(lcPlot).noquote() << "Fragment shader failed to compile:\n" << program_->log();
    }
  }
  else
  {
    // 2 - Fragment: apply color
    if(!program_->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                          R"(#version 450 core
             out vec4 fragColor;
             uniform vec4 u_color;
             void main() { fragColor = u_color; })"))
    {
      qFatal(lcPlot).noquote() << "Fragment shader failed to compile:\n" << program_->log();
    }
  }

  if(!program_->link())
  {
    qFatal(lcPlot).noquote() << "Shader program failed to link:\n" << program_->log();
  }

  program_->bind();
  ctx_.vbo._glVertexAttribPointer();
  program_->release();
  return true;
}

void LinePlotSeries::draw()
{
  program_->bind();

  if(isDirty())
  {
    const auto rng = updateDataProvider();

    if(rng.size() != ctx_.vbo._current_range.size())
    {
      ctx_.data.fromNdc.setToIdentity();
      ctx_.data.fromNdc.viewport(0.0,
                                 ctx_.data.scaleMin,
                                 (rng.size_bytes() / double(ctx_.vbo.elem_size)) - 1.0,
                                 ctx_.data.scaleMax - ctx_.data.scaleMin);
      ctx_.data.toNdc = ctx_.data.fromNdc.inverted();
      updateTransforms();
    }

    if(rng.data() != ctx_.vbo._current_range.data() or rng.size() != ctx_.vbo._current_range.size())
    {
      const GLuint offset = (rng.data() - ctx_.vbo.full_range().data()); // / ctx_.vbo.stride;

      ctx_.vbo._current_range =
          std::span<std::byte>(ctx_.vbo.full_range().data() + offset, rng.size_bytes());
    }
    setDirty(false);
  }

  const GLuint byte_offset = ctx_.vbo.current_byte_offset();
  const GLuint byte_count  = ctx_.vbo.current_byte_count();
  glUniform1ui(program_->uniformLocation("u_byteOffset"), byte_offset);
  glUniform1ui(program_->uniformLocation("u_byteCount"), byte_count);
  glUniform1ui(program_->uniformLocation("u_sampleStride"), ctx_.vbo.stride);
  program_->setUniformValue("u_color", lineColor_);
  program_->setUniformValue("u_thickness", thickness_ /* 0.0015f */);
  program_->setUniformValue("u_glow", glow_ /* 10.0f */);
  // program_->setUniformValue("u_dataToNdc", ctx_.data.toNdc);
  program_->setUniformValue("u_dataToNdc", ctx_.unit.dataToNdc);
  program_->setUniformValue("u_viewTransform", ctx_.view.transform);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ctx_.vbo._gl_handle);

  glPushAttrib(GL_ENABLE_BIT);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POLYGON_SMOOTH);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glLineWidth(1.0f);

  glDrawArrays(GL_LINE_STRIP, 0, byte_count / ctx_.vbo.stride);
  if(const auto e = glsl::checkError(); not e.empty())
  {
    qWarning() << "LineSeries glDrawArrays error:" << e;
  }
  glPopAttrib();
  program_->release();
}
