
#version 450

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

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

out vec2 vLineUV;

void main()
{
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;

    // direction of the line
    vec2 dir = normalize(p1.xy - p0.xy);

    // perpendicular vector for thickness
    vec2 normal = vec2(-dir.y, dir.x) * u_thickness * 2.0;

    vec2 n0 = p0.xy - normal;
    vec2 n1 = p0.xy + normal;
    vec2 n2 = p1.xy - normal;
    vec2 n3 = p1.xy + normal;

    // emit 4 vertices (two triangles)
    vLineUV = vec2(0.0, -1.0);
    gl_Position = vec4(n0, 0.0, 1.0);
    EmitVertex();

    vLineUV = vec2(0.0, 1.0);
    gl_Position = vec4(n1, 0.0, 1.0);
    EmitVertex();

    vLineUV = vec2(1.0, -1.0);
    gl_Position = vec4(n2, 0.0, 1.0);
    EmitVertex();

    vLineUV = vec2(1.0, 1.0);
    gl_Position = vec4(n3, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
