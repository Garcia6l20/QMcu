#version 450

layout(location = 0) in vec2 in_position;
layout(location = 0) out vec2 vPos;

layout(push_constant) uniform UBO {
    mat4 mvp;
    vec4 color;
    vec2 size;
    float border;
    float radius;
} ubo;

void main() {
    vPos = vec2(in_position.x * ubo.size.x, in_position.y * ubo.size.y);
    gl_Position = ubo.mvp * vec4(vPos, 0.0, 1.0);
    // gl_Position = vec4(gl_VertexIndex, gl_VertexIndex, 0.0, 1.0);
}
