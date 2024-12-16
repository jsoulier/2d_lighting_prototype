#version 450

#include "config.h"

layout(location = 0) in vec2 i_uv;
layout(location = 0) out float o_light;
layout(set = 2, binding = 0) uniform sampler2D s_position;
layout(set = 2, binding = 1) uniform sampler2D s_edge;
layout(set = 2, binding = 2) buffer readonly t_lights
{
    vec4 b_lights[];
};
layout(set = 3, binding = 0) uniform t_matrix
{
    mat4 u_matrix;
};
layout(set = 3, binding = 1) uniform t_num_lights
{
    uint u_num_lights;
};

float raycast(
    const vec3 src,
    const vec3 dst,
    float spread,
    const bool edge)
{
    const float penetration = length(vec2(MODEL_SIZE, MODEL_SIZE));
    const float step = 1.0f;
    if (distance(dst, src) > spread)
    {
        return 0.0f;
    }
    vec3 direction = dst - src;
    spread = length(direction);
    if (spread < penetration + PENETRATION_BIAS)
    {
        return SPREAD_COEFFICIENT / (spread + SPREAD_COEFFICIENT);
    }
    direction = normalize(direction);
    for (float i = 1.0f; i < spread - penetration; i += step)
    {
        const vec3 position = src + direction * i;
        vec4 uv = u_matrix * vec4(position, 1.0f);
        uv.xyz /= uv.w;
        uv.xyz = uv.xyz * 0.5f + 0.5f;
        uv.y = 1.0f - uv.y;
        if (uv.x <= 0.0f || uv.y <= 0.0f || uv.x > 1.0f || uv.y > 1.0f)
        {
            break;
        }
        const vec3 neighbor = texture(s_position, uv.xy).xyz;
        float bias = 1.0f;
        if (edge)
        {
            bias = -EDGE_BIAS;
        }
        if (neighbor.y - bias > position.y)
        {
            return 0.0f;
        }
    }
    return SPREAD_COEFFICIENT / (spread + SPREAD_COEFFICIENT);
}

void main()
{
    const vec3 position = texture(s_position, i_uv).xyz;
    for (int i = 0; i < u_num_lights; i++)
    {
        vec2 uv = i_uv;
        uv.y = 1.0f - uv.y;
        const bool edge = texture(s_edge, uv).x > 0.5f;
        o_light = max(o_light, raycast(position, b_lights[i].xyz, b_lights[i].w, edge));
        if (o_light >= 1.0f)
        {
            break;
        }
    }
    o_light = min(o_light, 1.0f);
}