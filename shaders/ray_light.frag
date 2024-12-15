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

bool raycast(
    const vec3 src,
    const vec3 dst,
    float spread)
{
    if (distance(dst, src) > spread)
    {
        return false;
    }
    vec3 direction = dst - src;
    spread = min(length(direction), spread);
    direction = normalize(direction);
    const float penetration = length(vec2(MODEL_SIZE, MODEL_SIZE));
    const float step = 1.0f;
    for (float i = 0.0f; i < spread - penetration; i += step)
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
        if (neighbor.y - 1.0f > position.y)
        {
            return false;
        }
    }
    return true;
}

void main()
{
    const vec3 position = texture(s_position, i_uv).xyz;
    for (int i = 0; i < u_num_lights; i++)
    {
        if (raycast(position, b_lights[i].xyz, b_lights[i].w))
        {
            o_light += 0.5f;
        }
        if (o_light >= 1.0f)
        {
            break;
        }
    }
    o_light = min(o_light, 1.0f);
}