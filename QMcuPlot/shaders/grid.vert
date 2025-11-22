#version 450

layout(location = 0) in vec2 in_position;

layout(push_constant) uniform UBO {
    mat4 mvp;
    vec4 color;
    vec2 boundingSize;
} ubo;

void main() {
    gl_Position = ubo.mvp * vec4(in_position.x * ubo.boundingSize.x, in_position.y * ubo.boundingSize.y, 0.0, 1.0);
}
