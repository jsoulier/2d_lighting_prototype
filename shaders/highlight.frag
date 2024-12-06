#version 450

#include "config.h"

layout(location = 0) in vec3 i_position;
layout(location = 0) out vec4 o_color;

void main()
{
    if (i_position.y <= 4.0f)
    {
        const vec3 offset = abs(i_position);
        const int edge = MODEL_SIZE / 2 - 2;
        const int size = MODEL_SIZE / 2 - 4;
        if ((offset.x > edge && offset.z > size) || (offset.z > edge && offset.x > size))
        {
            o_color = vec4(1.0f);
        }
        else
        {
            discard;
        }
    }
    else
    {
        o_color = vec4(i_position / MODEL_SIZE * 2.0f, 0.5f);
    }
}