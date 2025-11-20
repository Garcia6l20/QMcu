#version 450

layout(push_constant) uniform UBO {
    mat4 mvp;
    vec4 color;
    vec2 boundingSize;
    uint ticks;
    uint dash;
    uint gap;
} ubo;

void main() {
    // const float r = ((gl_VertexIndex + 1) / float(ubo.ticks + 1)) * 2.0f - 1.0f;
    const float r = (gl_VertexIndex + 1)/ float(ubo.ticks + 1);
    gl_Position = vec4(r, r, 0.0, 1.0);
}
