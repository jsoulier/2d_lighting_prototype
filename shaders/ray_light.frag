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
    vec3 direction = dst - src;
    const float step = 1.0f;
    const float intensity = 20.0f;
    const float spread2 = length(direction.xz);
    const float spread3 = length(direction);
    const float penetration2 = length(vec2(MODEL_SIZE, MODEL_SIZE)) / 2.0f;
    const float penetration3 = penetration2 * spread3 / spread2;
    if (spread2 > spread)
    {
        return 0.0f;
    }
    if (spread2 < penetration2)
    {
        return intensity / (spread2 + intensity);
    }
    direction = normalize(direction);
    for (float i = 0.0f; i < spread3 - penetration3; i += step)
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
            return 0.0f;
        }
    }
    return intensity / (spread2 + intensity);
}

void main()
{
    const vec3 position = texture(s_position, i_uv).xyz;
    for (int i = 0; i < u_num_lights; i++)
    {
        const vec3 dst = b_lights[i].xyz;
        const float spread = b_lights[i].w;
        o_light = max(o_light, raycast(position, dst, spread));
        if (o_light >= 1.0f)
        {
            break;
        }
    }
    o_light = min(o_light, 1.0f);
}