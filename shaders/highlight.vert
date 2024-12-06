#version 450

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec2 i_uv;
layout(location = 2) in vec3 i_normal;
layout(location = 0) out vec3 o_position;
layout(set = 1, binding = 0) uniform t_matrix
{
    mat4 u_matrix;
};
layout(set = 1, binding = 1) uniform t_instance
{
    vec3 u_instance;
};

void main()
{
    o_position = i_position;
    gl_Position = u_matrix * vec4(u_instance + o_position, 1.0f);
    gl_Position.z -= 0.001f;
}