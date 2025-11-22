#version 450

// layout(location = 0) in vec3 vLineUV;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform UBO {
    mat4 mvp;
    vec4 color;
    vec2 boundingSize;
    uint ticks;
    uint dash;
    uint gap;
} ubo;


void main() {
    // const bool vertical = (vLineUV.z > 0.0);

    // // Convert the NDC coordinate (-1..+1) into a pixel coordinate (0..size)
    // const float coord = (vertical ? vLineUV.y : vLineUV.x);
    // const float size  = vertical ? ubo.boundingSize.y : ubo.boundingSize.x;

    // // map from [-1,+1] to [0, size] in pixels
    // const float pixelPos = (coord * 0.5 + 0.5) * size;

    // // periodic dash pattern in pixel space
    // const float period = float(ubo.dash + ubo.gap);
    // const float pattern = mod(pixelPos, period);

    // if (pattern > float(ubo.dash))
    //     discard; // or outColor.a *= 0.0;

    outColor = ubo.color;
}
