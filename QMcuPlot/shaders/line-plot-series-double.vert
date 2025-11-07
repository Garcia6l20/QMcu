#version 450

layout(set = 1, binding = 0) buffer InputData {
    double data[];
} inData;

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

layout(location = 0) out vec4 vPosNdc;



void main() {
    const uint byteIndex = ubo.byteOffset + uint(gl_VertexIndex) * ubo.sampleStride;
    const float rawY = float(inData.data[byteIndex / 8]);

    const vec4 raw = vec4(float(gl_VertexIndex), rawY, 0.0, 1.0);
    const vec4 ndc = ubo.dataToNdc * raw;

    // Apply zoom/pan
    const vec4 view = ubo.viewTransform * ndc;

    const vec4 pixel = vec4(
        (((view.x + 1.0) * 0.5) * ubo.boundingSize.x),
        ((1.0 - view.y) * 0.5) * ubo.boundingSize.y, // flip Y for top-left origin
        0.0, 1.0);

    gl_Position = ubo.mvp * pixel;

    vPosNdc = vec4(view.xy, 0.0, 1.0);
}