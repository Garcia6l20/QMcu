#version 450

layout(set = 1, binding = 0) buffer InputData {
    uint data[];
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


// 8-bit unsigned
uint decodeU8(uint byteIndex) {
    const uint wordIndex = byteIndex / 4u;
    const uint byteInWord = byteIndex % 4u;
    return (inData.data[wordIndex] >> (byteInWord * 8u)) & 0xFFu;
}

// 8-bit signed
int decodeI8(uint byteIndex) {
    const uint uval = decodeU8(byteIndex);
    return (int(uval) << 8) >> 8; // sign-extend
}

// 16-bit unsigned (little-endian)
uint decodeU16(uint byteIndex) {
    const uint b0 = decodeU8(byteIndex + 0u);
    const uint b1 = decodeU8(byteIndex + 1u);
    return b0 | (b1 << 8u);
}

// 16-bit signed (little-endian)
int decodeI16(uint byteIndex) {
    const uint uval = decodeU16(byteIndex);
    return (int(uval) << 16) >> 16; // sign-extend
}

// 32-bit unsigned (little-endian)
uint decodeU32(uint byteIndex) {
    const uint b0 = decodeU8(byteIndex + 0u);
    const uint b1 = decodeU8(byteIndex + 1u);
    const uint b2 = decodeU8(byteIndex + 2u);
    const uint b3 = decodeU8(byteIndex + 3u);
    return b0 | (b1 << 8u) | (b2 << 16u) | (b3 << 24u);
}

// 32-bit signed (little-endian)
int decodeI32(uint byteIndex) {
    return int(decodeU32(byteIndex));
}


void main() {
    const uint byteIndex = ubo.byteOffset + uint(gl_VertexIndex) * ubo.sampleStride;
    const float rawY = decodeU8(byteIndex);

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