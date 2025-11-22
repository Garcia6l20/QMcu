#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform UBO {
    mat4 mvp;
    vec4 color;
    vec2 boundingSize;
} ubo;


void main() {
    outColor = ubo.color;
}
