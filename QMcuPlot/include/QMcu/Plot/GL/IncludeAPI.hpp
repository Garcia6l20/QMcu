#pragma once

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace glsl
{
inline bool hasIncludeExt()
{
  auto ctx = QOpenGLContext::currentContext();
  if(!ctx)
    return false;
  const std::string_view exts = (const char*)ctx->functions()->glGetString(GL_EXTENSIONS);
  return exts.contains("GL_ARB_shading_language_include");
}

struct IncludeAPI
{
  PFNGLNAMEDSTRINGARBPROC          glNamedStringARB          = nullptr;
  PFNGLDELETENAMEDSTRINGARBPROC    glDeleteNamedStringARB    = nullptr;
  PFNGLCOMPILESHADERINCLUDEARBPROC glCompileShaderIncludeARB = nullptr;
  bool                             valid                     = false;

  void registerInclude(std::string_view virtualPath, std::string_view source) const
  {
    glNamedStringARB(GL_SHADER_INCLUDE_ARB,
                     (GLsizei)virtualPath.size(),
                     virtualPath.data(),
                     (GLsizei)source.size(),
                     source.data());
  }

  void compileIncludePaths(GLuint prog, auto... paths)
  {
    char const* data[sizeof...(paths)];

    [&]<auto... I>(std::index_sequence<I...>)
    {
      ((data[I] = std::string_view{paths...[I]}.data()), ...);
    }(std::make_index_sequence<sizeof...(paths)>());
    glCompileShaderIncludeARB(prog, sizeof...(paths), data, nullptr);
  }
};

inline IncludeAPI getIncludeApi()
{
  IncludeAPI api;
  auto       ctx = QOpenGLContext::currentContext();
  if(!ctx)
    return api;

  auto proc = [&](const char* name) { return ctx->getProcAddress(QByteArray(name)); };

  api.glNamedStringARB = reinterpret_cast<PFNGLNAMEDSTRINGARBPROC>(proc("glNamedStringARB"));
  api.glDeleteNamedStringARB =
      reinterpret_cast<PFNGLDELETENAMEDSTRINGARBPROC>(proc("glDeleteNamedStringARB"));
  api.glCompileShaderIncludeARB =
      reinterpret_cast<PFNGLCOMPILESHADERINCLUDEARBPROC>(proc("glCompileShaderIncludeARB"));

  api.valid = api.glNamedStringARB && api.glDeleteNamedStringARB && api.glCompileShaderIncludeARB;
  return api;
}
} // namespace glsl