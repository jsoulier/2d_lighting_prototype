/* TODO: could perform better in a compute shader by running the raycasts in parallel */

#version 450

#include "config.h"

layout(location = 0) in vec2 i_uv;
layout(location = 0) out float o_light;
layout(set = 2, binding = 0) uniform sampler2D s_position;
layout(set = 2, binding = 1) uniform sampler2D s_normal;
layout(set = 2, binding = 2) uniform sampler2D s_ray_position_front;
layout(set = 2, binding = 3) uniform sampler2D s_ray_position_back;
layout(set = 2, binding = 4) uniform sampler2D s_sun_depth;
layout(set = 2, binding = 5) buffer readonly t_lights
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
    const vec2 uv,
    const vec3 src,
    const vec3 dst,
    const vec3 normal,
    const float spread)
{
    /* above light source */
    if (src.y - 1.0f > dst.y)
    {
        return 0.0f;
    }
    vec3 direction = dst - src;
    /* TODO: with texel alignment and PCF, it seems like I can raise the step
    size without any noticeable loss in accuracy. should verify */
    const float step1 = 1.0f;
    const vec2 step2 = step1 / vec2(textureSize(s_ray_position_front, 0));
    const float intensity = spread / 4.0f;
    const float spread2 = length(direction.xz);
    /* out of effective range */
    if (spread2 > spread)
    {
        return 0.0f;
    }
    const float spread3 = length(direction);
    const float penetration2 = length(vec2(MODEL_SIZE, MODEL_SIZE)) / 2.0f;
    const float penetration3 = penetration2 * spread3 / spread2;
    direction = normalize(direction);
    /* in light source */
    if (spread2 < penetration2)
    {
        return intensity / (spread2 + intensity);
    }
    /* is side face and facing away */
    else if (normal.y < 0.1f && dot(direction.xz, normal.xz) < 0.0f)
    {
        return 0.0f;
    }
    /* bias forwards slightly to ensure walls get lighting */
    const float bias = 1.0f;
    vec2 j = bias / step1 * step2;
    for (float i = bias; i < spread3 - penetration3; i += step1, j += step2)
    {
        const vec3 position = src + direction * i;
        /* align to texel */
        vec2 neighbor_uv = uv + direction.xz * j;
        const vec2 size = vec2(textureSize(s_ray_position_front, 0));
        neighbor_uv = floor(neighbor_uv * size) / size + (1.0f / size) * 0.5f;
        /* check if between front and back faces */
        const vec3 neighbor_front = texture(s_ray_position_front, neighbor_uv).xyz;
        const vec3 neighbor_back = texture(s_ray_position_back, neighbor_uv).xyz;
        if (position.y > neighbor_back.y && position.y < neighbor_front.y)
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
    const vec3 position = texture(s_position, i_uv).xyz;
    const vec3 normal = texture(s_normal, i_uv).xyz;
    vec4 uv = u_ray_matrix * vec4(position, 1.0f);
    uv.xy = uv.xy * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    o_light = 0.2f;
    o_light = max(o_light, get_sun_light(position, normal) / 2.0f);
    for (int i = 0; i < u_num_lights && o_light < 1.0f; i++)
    {
        o_light = max(o_light, get_ray_light(
            uv.xy,
            position,
            b_lights[i].xyz,
            normal,
            b_lights[i].w));
    }
}