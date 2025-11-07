#version 450


layout(binding = 0) uniform UBO {
    mat4 mvp;
    
    mat4 dataToNdc;       // data -> NDC
    mat4 viewTransform;   // zoom & pan in NDC space

    vec4  color;    // base color
    float thickness;
    float glow;

    uint byteCount;     // byte count
    uint byteOffset;    // byte offset
    uint sampleStride;  // sample stride

    uint tid;
} ubo;

in vec2 vLineUV;
out vec4 fragColor;

void main() {
    float dist = abs(vLineUV.y); // distance from line center

    float core = smoothstep(ubo.thickness, 0.0, dist);
    float halo = exp(-pow(dist / ubo.thickness, 2.0) * 5.0) * ubo.glow;

    float intensity = core + halo;
    fragColor = vec4(ubo.color.rgb * intensity, clamp(intensity, 0.0, 1.0));
}
