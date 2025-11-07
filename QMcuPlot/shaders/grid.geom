#version 450

layout(points) in;
layout(line_strip, max_vertices = 4) out;

layout(location = 0) out vec3 vLineUV;

layout(push_constant) uniform UBO {
    mat4 mvp;
    vec4 color;
    vec2 boundingSize;
    uint ticks;
    uint dash;
    uint gap;
} ubo;

void main() {
    const float r = gl_in[0].gl_Position.x;
    
    // Vertical line
    gl_Position = ubo.mvp * vec4(r * ubo.boundingSize.x, 0.0, 0.0, 1.0);
    // gl_Position = ubo.mvp * vec4(r, -1.0, 0.0, 1.0);
    // gl_Position = vec4(r, -1.0, 0.0, 1.0);
    vLineUV = vec3(r, -1.0, 1.0); // z used to mark as vertical for grid.frag
    EmitVertex();

    gl_Position = ubo.mvp * vec4(r * ubo.boundingSize.x, ubo.boundingSize.y, 0.0, 1.0);
    // gl_Position = ubo.mvp * vec4(r, 1.0, 0.0, 1.0);
    // gl_Position = vec4(r, 1.0, 0.0, 1.0);
    vLineUV = vec3(r, 1.0, 1.0);
    EmitVertex();
    EndPrimitive();

    // Horizontal line
    gl_Position = ubo.mvp * vec4(0, r * ubo.boundingSize.y, 0.0, 1.0);
    // gl_Position = ubo.mvp * vec4(-1.0, r, 0.0, 1.0);
    // gl_Position = vec4(-1.0, r, 0.0, 1.0);
    vLineUV = vec3(-1.0, r, -1.0); // z used to mark as horizontal for grid.frag
    EmitVertex();

    gl_Position = ubo.mvp * vec4(ubo.boundingSize.x, r * ubo.boundingSize.y, 0.0, 1.0);
    // gl_Position = ubo.mvp * vec4(1.0, r, 0.0, 1.0);
    // gl_Position = vec4(1.0, r, 0.0, 1.0);
    vLineUV = vec3(1.0, r, -1.0);
    EmitVertex();
    EndPrimitive();
}
