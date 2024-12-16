#version 450

#include "config.h"

layout(location = 0) in vec2 i_uv;
layout(location = 0) out float o_light;
layout(set = 2, binding = 0) uniform sampler2D s_default_position;
layout(set = 2, binding = 1) uniform sampler2D s_default_normal;
layout(set = 2, binding = 2) uniform sampler2D s_ray_position;
layout(set = 2, binding = 3) uniform sampler2D s_sun_depth;
layout(set = 2, binding = 4) buffer readonly t_lights
{
    vec4 b_lights[];
};
layout(set = 3, binding = 0) uniform t_ray_matrix
{
    mat4 u_ray_matrix;
};
layout(set = 3, binding = 1) uniform t_sun_matrix
{
    mat4 u_sun_matrix;
};
layout(set = 3, binding = 2) uniform t_sun_direction
{
    vec3 u_sun_direction;
};
layout(set = 3, binding = 3) uniform t_num_lights
{
    uint u_num_lights;
};

float get_ray_light(
    const vec3 src,
    const vec3 dst,
    float spread)
{
    vec3 direction = dst - src;
    const float step = 1.0f;
    const float intensity = spread / 4.0f;
    const float spread2 = length(direction.xz);
    if (spread2 > spread)
    {
        return 0.0f;
    }
    const float spread3 = length(direction);
    const float penetration2 = length(vec2(MODEL_SIZE, MODEL_SIZE)) / 2.0f;
    const float penetration3 = penetration2 * spread3 / spread2;
    if (spread2 < penetration2)
    {
        return intensity / (spread2 + intensity);
    }
    direction = normalize(direction);
    for (float i = 0.0f; i < spread3 - penetration3; i += step)
    {
        const vec3 position = src + direction * i;
        vec4 uv = u_ray_matrix * vec4(position, 1.0f);
        uv.xyz /= uv.w;
        uv.xyz = uv.xyz * 0.5f + 0.5f;
        uv.y = 1.0f - uv.y;
        const vec3 neighbor = texture(s_ray_position, uv.xy).xyz;
        if (neighbor.y - 1.0f > position.y)
        {
            return 0.0f;
        }
    }
    return intensity / (spread2 + intensity);
}

float get_sun_light(
    const vec3 position,
    const vec3 normal)
{
    const float sun = max(dot(u_sun_direction, -normal), 0.0f);
    if (sun <= 0.0f)
    {
        return 0.0f;
    }
    vec4 uv = u_sun_matrix * vec4(position, 1.0f);
    const float depth = uv.z;
    uv.xy = uv.xy * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    const float nearest = texture(s_sun_depth, uv.xy).x;
    return float(depth - 0.005f < nearest) / 2.0f;
}

void main()
{
    const vec3 position = texture(s_default_position, i_uv).xyz;
    const vec3 normal = texture(s_default_normal, i_uv).xyz;
    vec4 ray_uv = u_ray_matrix * vec4(position, 1.0f);
    ray_uv.xy = ray_uv.xy * 0.5f + 0.5f;
    ray_uv.y = 1.0f - ray_uv.y;
    const vec3 ray_position = texture(s_ray_position, ray_uv.xy).xyz;
    o_light = 0.2f;
    o_light = max(o_light, get_sun_light(position, normal) / 2.0f);
    for (int i = 0; i < u_num_lights; i++)
    {
        if (o_light >= 1.0f)
        {
            break;
        }
        const vec3 dst = b_lights[i].xyz;
        const float spread = b_lights[i].w;
        o_light = max(o_light, get_ray_light(ray_position, dst, spread));
    }
}