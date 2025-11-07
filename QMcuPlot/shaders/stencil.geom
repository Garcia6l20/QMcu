#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 64) out;

layout(push_constant) uniform UBO {
    mat4 mvp;
    vec4 color;
    vec2 size;
    float border;
    float radius;
} ubo;

layout(location = 0) out vec2 vPos;

void main() {

    // bottom-left
    vPos = vec2(0.0, 0.0);
    gl_Position = ubo.mvp * vec4(vPos, 0.0, 1.0);
    EmitVertex();

    // bottom-right
    vPos = vec2(float(ubo.size.x), 0.0);
    gl_Position = ubo.mvp * vec4(vPos, 0.0, 1.0);
    EmitVertex();

    // top-left
    vPos = vec2(0.0, float(ubo.size.y));
    gl_Position = ubo.mvp * vec4(vPos, 0.0, 1.0);
    EmitVertex();

    // top-right
    vPos = vec2(float(ubo.size.x), float(ubo.size.y));
    gl_Position = ubo.mvp * vec4(vPos, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}