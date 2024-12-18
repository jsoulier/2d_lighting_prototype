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

/**
 * @param uv The uvs into the topdown ortho texture
 * @param psrc The world space coordinates of the fragment (perspective)
 * @param osrc The world space coordinates of the fragment (ortho)
 * The osrc parameter loses the height of the fragment. e.g. Fragments under a
 * tree may be interpreted as the fragments on top of the tree
 * @param dst The world space coordinates of the light
 * @param normal
 * @param spread
 */
float get_ray_light(
    const vec2 uv,
    const vec3 psrc,
    const vec3 osrc,
    const vec3 dst,
    const vec3 normal,
    const float spread)
{
    /* Above the light */
    if (osrc.y - 1.0f > dst.y)
    {
        return 0.0f;
    }
    vec3 pdirection = dst - psrc;
    vec3 odirection = dst - osrc;
    const float step1 = 1.0f;
    const vec2 step2 = 1.0f / vec2(textureSize(s_ray_position_front, 0));
    const float intensity = spread / 4.0f;
    const float spread2 = length(odirection.xz);
    /* Out of the effective range */
    if (spread2 > spread)
    {
        return 0.0f;
    }
    const float spread3 = length(odirection);
    const float penetration2 = length(vec2(MODEL_SIZE, MODEL_SIZE)) / 2.0f;
    const float penetration3 = penetration2 * spread3 / spread2;
    pdirection = normalize(pdirection);
    odirection = normalize(odirection);
    /* Inside the light model */
    if (spread2 < penetration2)
    {
        return intensity / (spread2 + intensity);
    }
    /* Is side face and facing away from light */
    else if (normal.y < 0.1f && dot(odirection.xz, normal.xz) < 0.0f)
    {
        return 0.0f;
    }
    vec2 j = vec2(0.0f);
    for (float i = 0.0f; i < spread3 - penetration3; i += step1, j += step2)
    {
        const vec3 oposition = osrc + odirection * i;
        const vec2 neighbor_uv = uv + odirection.xz * j;
        const vec3 neighbor_front = texture(s_ray_position_front, neighbor_uv).xyz;
        /* Heightmap check */
        if (neighbor_front.y - 1.0f > oposition.y)
        {
            return 0.0f;
        }
        /* Required because raycasts can travel along walls, putting them inside it */
        if (i < 16.0f)
        {
            continue;
        }
        const vec3 pposition = psrc + pdirection * i;
        const vec3 neighbor_back = texture(s_ray_position_back, neighbor_uv).xyz;
        /* If between front and back face */
        if (pposition.y - 1.0f > neighbor_back.y && pposition.y + 1.0f < neighbor_front.y)
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
    const vec3 pposition = texture(s_position, i_uv).xyz;
    const vec3 normal = texture(s_normal, i_uv).xyz;
    vec4 uv = u_ray_matrix * vec4(pposition, 1.0f);
    uv.xy = uv.xy * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    const vec3 oposition = texture(s_ray_position_front, uv.xy).xyz;
    o_light = 0.2f;
    o_light = max(o_light, get_sun_light(pposition, normal) / 2.0f);
    for (int i = 0; i < u_num_lights && o_light < 1.0f; i++)
    {
        o_light = max(o_light, get_ray_light(
            uv.xy,
            pposition,
            oposition,
            b_lights[i].xyz,
            normal,
            b_lights[i].w));
    }
}