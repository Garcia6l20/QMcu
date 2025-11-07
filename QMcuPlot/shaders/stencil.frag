#version 450
layout(location = 0) in vec2 vPos;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform UBO {
    mat4 mvp;
    vec4 color;
    vec2 size;
    float border;
    float radius;
} ubo;

float sdRoundRect(vec2 p, vec2 b, float r) {
    vec2 d = abs(p - b*0.5) - (b*0.5 - vec2(r));
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0) - r;
}

void main() {
    // const vec2 min = vec2(0.0, 0.0);
    // const vec2 max = ubo.size;

    // // Corners
    // const vec2 topLeft     = min + vec2(ubo.radius, ubo.size.y - ubo.radius);
    // const vec2 topRight    = vec2(max.x - ubo.radius, max.y - ubo.radius);
    // const vec2 bottomLeft  = vec2(min.x + ubo.radius, min.y + ubo.radius);
    // const vec2 bottomRight = vec2(max.x - ubo.radius, min.y + ubo.radius);

    // // Outside rounded corners
    // if (vPos.x < topLeft.x && vPos.y > topLeft.y && distance(vPos, topLeft) > ubo.radius) discard;
    // if (vPos.x > topRight.x && vPos.y > topRight.y && distance(vPos, topRight) > ubo.radius) discard;
    // if (vPos.x < bottomLeft.x && vPos.y < bottomLeft.y && distance(vPos, bottomLeft) > ubo.radius) discard;
    // if (vPos.x > bottomRight.x && vPos.y < bottomRight.y && distance(vPos, bottomRight) > ubo.radius) discard;

    float dist = sdRoundRect(vPos, ubo.size, ubo.radius);

    if (dist > -ubo.border)
        discard; // outside rounded rect

    outColor = ubo.color;
}
