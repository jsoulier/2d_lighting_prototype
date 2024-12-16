#version 450

layout(location = 0) in vec4 i_position;
layout(location = 1) in vec2 i_uv;
layout(location = 2) in vec3 i_normal;
layout(location = 0) out vec4 o_color;
layout(location = 1) out vec4 o_position;
layout(location = 2) out vec4 o_normal;
layout(set = 2, binding = 0) uniform sampler2D s_palette;

void main()
{
    o_color = texture(s_palette, i_uv);
    o_position = i_position;
    o_normal = vec4(i_normal, 0.0);
}