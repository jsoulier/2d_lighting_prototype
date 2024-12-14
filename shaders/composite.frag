#version 450

layout(location = 0) in vec2 i_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D s_main_color;
layout(set = 2, binding = 1) uniform sampler2D s_main_position;
layout(set = 2, binding = 2) uniform sampler2D s_main_normal;
layout(set = 2, binding = 3) uniform sampler2D s_topdown_light;
layout(set = 2, binding = 4) uniform sampler2D s_sun_depth;
layout(set = 3, binding = 0) uniform t_topdown
{
    mat4 u_topdown;
};
layout(set = 3, binding = 1) uniform t_sun
{
    mat4 u_sun;
};
layout(set = 3, binding = 2) uniform t_sun_direction
{
    vec3 u_sun_direction;
};

float get_sun(
    const vec3 position)
{
    vec4 uv = u_sun * vec4(position, 1.0f);
    const float depth = uv.z;
    uv.xy = uv.xy * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    const int kernel = 5;
    const vec2 size = 1.0f / vec2(textureSize(s_sun_depth, 0));
    float sun = 0.0f;
    for (int x = -kernel; x <= kernel; x++)
    {
        for (int y = -kernel; y <= kernel; y++)
        {
            const vec2 neighbor_uv = uv.xy + vec2(x, y) * size;
            const float neighbor_depth = texture(s_sun_depth, neighbor_uv).x;
            sun += float(depth - 0.01f < neighbor_depth);
        }
    }
    sun /= (kernel * 2 + 1) * (kernel * 2 + 1);
    return sun;
}

float get_ssao(
    const vec4 color,
    const vec3 normal)
{
    const int kernel = 5;
    const vec2 size = 1.0f / vec2(textureSize(s_main_normal, 0));
    float ssao = 0.0f;
    for (int x = -kernel; x <= kernel; x++)
    {
        for (int y = -kernel; y <= kernel; y++)
        {
            const vec2 neighbor_uv = i_uv + vec2(x, y) * size;
            const vec3 neighbor_normal = texture(s_main_normal, neighbor_uv).xyz;
            if (dot(normal, neighbor_normal) < 0.9f)
            {
                ssao += 1.0f;
                continue;
            }
            const vec4 neighbor_color = texture(s_main_color, neighbor_uv);
            if (distance(color, neighbor_color) > 0.1f)
            {
                ssao += 1.0f;
                continue;
            }
        }
    }
    ssao /= (kernel * 2 + 1) * (kernel * 2 + 1);
    return ssao;
}

float get_light(
    const vec3 position)
{
    vec4 uv = u_topdown * vec4(position, 1.0f);
    uv.xy = uv.xy * 0.5f + 0.5f;
    const int kernel = 5;
    const vec2 size = 1.0f / vec2(textureSize(s_topdown_light, 0)) * 1.0f;
    float light = 0.0f;
    for (int x = -kernel; x <= kernel; x++)
    {
        for (int y = -kernel; y <= kernel; y++)
        {
            const vec2 neighbor_uv = uv.xy + vec2(x, y) * size;
            const float neighbor_light = texture(s_topdown_light, neighbor_uv).x;
            light += neighbor_light;
        }
    }
    light /= (kernel * 2 + 1) * (kernel * 2 + 1);
    return light;
}

void main()
{
    const vec4 color = texture(s_main_color, i_uv);
    const vec3 position = texture(s_main_position, i_uv).xyz;
    const vec3 normal = texture(s_main_normal, i_uv).xyz;
    const float sun = max(dot(u_sun_direction, -normal), 0.0f);
    float light = 0.4f;
    if (sun > 0.0f)
    {
        light = max(light, get_sun(position) / 2.0f);
    }
    light = max(light, get_light(position) * 2.0f);
    light = min(light, 1.2f);
    light -= get_ssao(color, normal) / 2.0f;
    o_color = color * light;
}