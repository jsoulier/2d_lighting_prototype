#version 450

#include "config.h"

layout(location = 0) in vec2 i_uv;
layout(location = 0) out float o_light;
layout(set = 2, binding = 0) uniform sampler2D s_position;
layout(set = 2, binding = 1) buffer readonly t_lights
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
    float spread)
{
    /* TODO: increment by texel size in world space */
    const float step = 1.0f;
    if (distance(dst.xz, src.xz) > spread)
    {
        return 0.0f;
    }
    vec3 direction = dst - src;
    direction.y = 0;
    spread = length(direction.xz);
    if (spread < RAY_END)
    {
        return SPREAD_COEFFICIENT / (spread + SPREAD_COEFFICIENT);
    }
    direction = normalize(direction);

    /* trace backwards along axes to prevent far edges from sampling light */
    vec3 axis;
    axis = vec3(sign(direction.x), 0.0f, 0.0f);
    for (float i = RAY_START; i < 0.0f; i += step)
    {
        const vec3 position = src + axis * i;
        vec4 uv = u_matrix * vec4(position, 1.0f);
        uv.xyz /= uv.w;
        uv.xyz = uv.xyz * 0.5f + 0.5f;
        uv.y = 1.0f - uv.y;
        const vec3 neighbor = texture(s_position, uv.xy).xyz;
        if (neighbor.y + 1.0f < src.y)
        {
            return 0.0f;
        }
    }
    axis = vec3(0.0f, 0.0f, sign(direction.z));
    for (float i = RAY_START; i < 0.0f; i += step)
    {
        const vec3 position = src + axis * i;
        vec4 uv = u_matrix * vec4(position, 1.0f);
        uv.xyz /= uv.w;
        uv.xyz = uv.xyz * 0.5f + 0.5f;
        uv.y = 1.0f - uv.y;
        const vec3 neighbor = texture(s_position, uv.xy).xyz;
        if (neighbor.y + 1.0f < src.y)
        {
            return 0.0f;
        }
    }

    for (float i = 0.0f; i < spread - RAY_END; i += step)
    {
        const vec3 position = src + direction * i;
        vec4 uv = u_matrix * vec4(position, 1.0f);
        uv.xyz /= uv.w;
        uv.xyz = uv.xyz * 0.5f + 0.5f;
        uv.y = 1.0f - uv.y;
        const vec3 neighbor = texture(s_position, uv.xy).xyz;
        if (neighbor.y - 1.0f > position.y)
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
        o_light = max(o_light, raycast(position, b_lights[i].xyz, b_lights[i].w));
        if (o_light >= 1.0f)
        {
            break;
        }
    }
    o_light = min(o_light, 1.0f);
}