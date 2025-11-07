#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform UBO {
  vec4 u_color;
} ubo;

void main() {
  outColor = ubo.u_color;
}
