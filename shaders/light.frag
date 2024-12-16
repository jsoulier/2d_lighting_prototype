#version 450

layout(location = 0) in vec2 i_uv;
layout(location = 0) out float o_light;
layout(set = 2, binding = 0) uniform sampler2D s_default_position;
layout(set = 2, binding = 1) uniform sampler2D s_default_normal;
layout(set = 2, binding = 2) uniform sampler2D s_ray_light;
layout(set = 2, binding = 3) uniform sampler2D s_sun_depth;
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

void main()
{
    const vec3 position = texture(s_default_position, i_uv).xyz;
    const vec3 normal = texture(s_default_normal, i_uv).xyz;
    const float sun = max(dot(u_sun_direction, -normal), 0.0f);
    o_light = 0.2f;
    if (sun > 0.0f)
    {
        vec4 uv = u_sun_matrix * vec4(position, 1.0f);
        const float depth = uv.z;
        uv.xy = uv.xy * 0.5f + 0.5f;
        uv.y = 1.0f - uv.y;
        const float nearest = texture(s_sun_depth, uv.xy).x;
        o_light = max(o_light, float(depth - 0.005f < nearest) / 2.0f);
    }
    vec4 uv = u_ray_matrix * vec4(position, 1.0f);
    uv.xy = uv.xy * 0.5f + 0.5f;
    o_light = max(o_light, texture(s_ray_light, uv.xy).x * 2.0f);
    o_light = min(o_light, 1.5f);
}