#version 450

layout(location = 0) in vec4 i_position;
layout(location = 1) in vec2 i_uv;
layout(location = 2) in vec3 i_normal;
layout(location = 0) out vec4 o_position;

void main()
{
    o_position = i_position;
}