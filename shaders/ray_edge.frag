#version 450

layout(location = 0) in vec2 i_uv;
layout(location = 0) out float o_edge;
layout(set = 2, binding = 0) uniform sampler2D s_position;

void main()
{
    const float y = texture(s_position, i_uv).y;
    const int kernel = 1;
    const vec2 size = 1.0f / vec2(textureSize(s_position, 0));
    for (int x = -kernel; x <= kernel; x++)
    {
        for (int z = -kernel; z <= kernel; z++)
        {
            const vec2 uv = i_uv + vec2(x, z) * size;
            const float neighbor_y = texture(s_position, uv).y;
            if (abs(y - neighbor_y) > 1.0f)
            {
                o_edge = 1.0f;
                return;
            }
        }
    }
}