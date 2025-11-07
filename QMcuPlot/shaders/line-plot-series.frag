#version 450


layout(binding = 0) uniform UBO {
    mat4 mvp;
    
    mat4 dataToNdc;       // data -> NDC
    mat4 viewTransform;   // zoom & pan in NDC space

    vec4  color;    // base color
    
    vec2 boundingSize;

    float thickness;
    float glow;

    uint byteCount;     // byte count
    uint byteOffset;    // byte offset
    uint sampleStride;  // sample stride

    uint tid;
} ubo;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = ubo.color;
}