#version 450

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec2 i_uv;
layout(location = 2) in vec3 i_normal;
layout(location = 3) in vec4 i_instance;
layout(location = 0) out vec4 o_position;
layout(location = 1) out vec2 o_uv;
layout(location = 2) out vec3 o_normal;
layout(set = 1, binding = 0) uniform t_matrix
{
    mat4 u_matrix;
};

void main()
{
    o_position = vec4(i_position + i_instance.xyz, 1.0);
    o_uv = i_uv;
    o_normal = i_normal;
    gl_Position = u_matrix * o_position;

    /* bias side facing tiles away to reduce z-fighting */
    const vec3 up = vec3(0.0f, 1.0f, 0.0f);
    const float factor = max(dot(up, o_normal), 0.0f);
    gl_Position.z -= factor * 0.001f;
}