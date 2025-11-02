#include <QMcu/Plot/PlotGrid.hpp>

#include <Logging.hpp>

#include <QColor>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

PlotGrid::PlotGrid(QObject* parent) : BasicRenderer{parent}
{
}

bool PlotGrid::allocateGL(QSize const& viewport)
{
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

  compute_ = std::make_unique<QOpenGLShaderProgram>();
  if(!compute_->addShaderFromSourceCode(QOpenGLShader::Compute,
                                        R"GLSL(#version 450 core
layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer VertexBuffer {
    vec4 vertices[];
};

uniform uint u_ticks;

void main() {
    const uint idx = gl_GlobalInvocationID.x;

    if(idx >= u_ticks) return;

    // space ticks evenly between -1 and +1 (excluded)
    const float step = 2.0 / float(u_ticks + 1u);
    const float ratio = -1.0 + step * float(idx + 1u);

    // horizontal line
    vertices[idx * 4 + 0] = vec4(ratio, -1.0, 0.0, 1.0);
    vertices[idx * 4 + 1] = vec4(ratio,  1.0, 0.0, 1.0);

    // vertical line
    vertices[idx * 4 + 2] = vec4(-1.0, ratio, 0.0, 1.0);
    vertices[idx * 4 + 3] = vec4( 1.0, ratio, 0.0, 1.0);
}
)GLSL"))
  {
    qFatal(lcPlot).noquote() << "Compute shader failed to compile:\n" << compute_->log();
  }
  if(!compute_->link())
  {
    qFatal(lcPlot).noquote() << "Compute shader program failed to link:\n" << compute_->log();
  }

  program_ = std::make_unique<QOpenGLShaderProgram>();
  if(!program_->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                        R"GLSL(#version 450 core
layout(location = 0) in vec4 in_position; // dummy input

void main() {
    gl_Position = in_position;
}
)GLSL"))
  {
    qFatal(lcPlot).noquote() << "Fragment shader failed to compile:\n" << program_->log();
  }

  if(!program_->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                        R"GLSL(#version 450 core
           out vec4 fragColor;
           uniform vec4 u_color;
           void main() { fragColor = u_color; })GLSL"))
  {
    qFatal(lcPlot).noquote() << "Fragment shader failed to compile:\n" << program_->log();
  }

  if(!program_->link())
  {
    qFatal(lcPlot).noquote() << "Shader program failed to link:\n" << program_->log();
  }

  assert(not glHandle_);

  static constexpr size_t MAX_VERTICES = 32;
  glGenBuffers(1, &glHandle_);
  glBindBuffer(GL_ARRAY_BUFFER, glHandle_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * MAX_VERTICES, nullptr, GL_STATIC_DRAW);
  return true;
}

void PlotGrid::draw()
{
  const GLuint TOTAL_VERTICES = ticks_ * 4;
  if(isDirty())
  {
    compute_->bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, glHandle_);
    // WARNING Qt uses glUniform1i here !!!!
    // program_->setUniformValue("u_ticks", GLuint(ticks_));
    glUniform1ui(compute_->uniformLocation("u_ticks"), ticks_);

    // Dispatch one workgroup per tick (or multiple invocations per workgroup)
    glDispatchCompute(ticks_, 1, 1);
    if(const auto e = glsl::checkError(); not e.empty())
    {
      qWarning() << "glDispatchCompute error:" << e;
    }
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    compute_->release();
  }

  program_->bind();
  program_->setUniformValue("u_color", color_);
  glBindBuffer(GL_ARRAY_BUFFER, glHandle_);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);

  glPushAttrib(GL_ENABLE_BIT);
  glLineStipple(1, 0b1110000000000111);
  glEnable(GL_LINE_STIPPLE);
  glLineWidth(0.5);
  glDrawArrays(GL_LINES, 0, TOTAL_VERTICES);
  if(const auto e = glsl::checkError(); not e.empty())
  {
    qWarning() << "glDrawArrays error:" << e;
  }
  glPopAttrib();
  program_->release();
}