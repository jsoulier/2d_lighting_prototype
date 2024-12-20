#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "camera.h"
#include "config.h"
#include "helpers.h"
#include "model.h"
#include "renderer.h"
#include "world.h"

enum
{
    TEXTURE_COLOR,
    TEXTURE_DEPTH,
    TEXTURE_POSITION,
    TEXTURE_NORMAL,
    TEXTURE_RAY_DEPTH_FRONT,
    TEXTURE_RAY_POSITION_FRONT,
    TEXTURE_RAY_DEPTH_BACK,
    TEXTURE_RAY_POSITION_BACK,
    TEXTURE_SUN_DEPTH,
    TEXTURE_LIGHT,
    TEXTURE_COMPOSITE,
    TEXTURE_COUNT,
};

enum
{
    SAMPLER_NEAREST,
    SAMPLER_LINEAR,
    SAMPLER_COUNT,
};

enum
{
    GRAPHICS_MODEL,
    GRAPHICS_RAY_MODEL_FRONT,
    GRAPHICS_RAY_MODEL_BACK,
    GRAPHICS_SUN_MODEL,
    GRAPHICS_HIGHLIGHT,
    GRAPHICS_LIGHT,
    GRAPHICS_COMPOSITE,
    GRAPHICS_COUNT,
};

enum
{
    COMPUTE_SAMPLER,
    COMPUTE_COUNT,
};

static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_GPUGraphicsPipeline* graphics[GRAPHICS_COUNT];
static SDL_GPUComputePipeline* computes[COMPUTE_COUNT];
static SDL_GPUTexture* textures[TEXTURE_COUNT];
static SDL_GPUSampler* samplers[SAMPLER_COUNT];
static SDL_GPUTransferBuffer* sampler_tbo;
static SDL_GPUBuffer* sampler_sbo;
static camera_t camera;
static camera_t ray_camera;
static camera_t sun_camera;
static uint32_t width;
static uint32_t height;
static uint32_t rwidth;
static uint32_t rheight;
static uint32_t bx;
static uint32_t by;
static uint32_t bwidth;
static uint32_t bheight;

static bool create_pipelines()
{
    assert(device);
    SDL_GPUGraphicsPipelineCreateInfo info[GRAPHICS_COUNT] = {0};
    info[GRAPHICS_MODEL] = (SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = load_shader(device, "model.vert"),
        .fragment_shader = load_shader(device, "model.frag"),
        .target_info =
        {
            .num_color_targets = 3,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            },
            {
                .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
            },
            {
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM,
            }},
            .has_depth_stencil_target = true,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        },
        .vertex_input_state =
        {
            .num_vertex_attributes = 5,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {{
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 0,
                .offset = sizeof(float) * 0,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .location = 1,
                .offset = sizeof(float) * 3,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 2,
                .offset = sizeof(float) * 5,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 3,
                .offset = sizeof(float) * 0,
                .buffer_slot = 1,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
                .location = 4,
                .offset = sizeof(float) * 3,
                .buffer_slot = 1,
            }},
            .num_vertex_buffers = 2,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {{
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .pitch = sizeof(float) * 8,
                .instance_step_rate = 0,
                .slot = 0,
            },
            {
                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                .pitch = sizeof(float) * 4,
                .instance_step_rate = 1,
                .slot = 1,
            }},
        },
        .depth_stencil_state =
        {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .rasterizer_state =
        {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        }
    };
    info[GRAPHICS_RAY_MODEL_FRONT] = (SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = load_shader(device, "model.vert"),
        .fragment_shader = load_shader(device, "ray_model.frag"),
        .target_info =
        {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
            }},
            .has_depth_stencil_target = true,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        },
        .vertex_input_state =
        {
            .num_vertex_attributes = 5,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {{
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 0,
                .offset = sizeof(float) * 0,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .location = 1,
                .offset = sizeof(float) * 3,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 2,
                .offset = sizeof(float) * 5,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 3,
                .offset = sizeof(float) * 0,
                .buffer_slot = 1,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
                .location = 4,
                .offset = sizeof(float) * 3,
                .buffer_slot = 1,
            }},
            .num_vertex_buffers = 2,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {{
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .pitch = sizeof(float) * 8,
                .instance_step_rate = 0,
                .slot = 0,
            },
            {
                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                .pitch = sizeof(float) * 4,
                .instance_step_rate = 1,
                .slot = 1,
            }},
        },
        .depth_stencil_state =
        {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .rasterizer_state =
        {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        }
    };
    info[GRAPHICS_RAY_MODEL_BACK] = (SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = load_shader(device, "model.vert"),
        .fragment_shader = load_shader(device, "ray_model.frag"),
        .target_info =
        {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
            }},
            .has_depth_stencil_target = true,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        },
        .vertex_input_state =
        {
            .num_vertex_attributes = 5,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {{
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 0,
                .offset = sizeof(float) * 0,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .location = 1,
                .offset = sizeof(float) * 3,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 2,
                .offset = sizeof(float) * 5,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 3,
                .offset = sizeof(float) * 0,
                .buffer_slot = 1,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
                .location = 4,
                .offset = sizeof(float) * 3,
                .buffer_slot = 1,
            }},
            .num_vertex_buffers = 2,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {{
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .pitch = sizeof(float) * 8,
                .instance_step_rate = 0,
                .slot = 0,
            },
            {
                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                .pitch = sizeof(float) * 4,
                .instance_step_rate = 1,
                .slot = 1,
            }},
        },
        .depth_stencil_state =
        {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .rasterizer_state =
        {
            .cull_mode = SDL_GPU_CULLMODE_FRONT,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        }
    };
    info[GRAPHICS_SUN_MODEL] = (SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = load_shader(device, "model.vert"),
        .fragment_shader = load_shader(device, "sun_model.frag"),
        .target_info =
        {
            .has_depth_stencil_target = true,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        },
        .vertex_input_state =
        {
            .num_vertex_attributes = 5,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {{
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 0,
                .offset = sizeof(float) * 0,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .location = 1,
                .offset = sizeof(float) * 3,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 2,
                .offset = sizeof(float) * 5,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 3,
                .offset = sizeof(float) * 0,
                .buffer_slot = 1,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
                .location = 4,
                .offset = sizeof(float) * 3,
                .buffer_slot = 1,
            }},
            .num_vertex_buffers = 2,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {{
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .pitch = sizeof(float) * 8,
                .instance_step_rate = 0,
                .slot = 0,
            },
            {
                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                .pitch = sizeof(float) * 4,
                .instance_step_rate = 1,
                .slot = 1,
            }},
        },
        .depth_stencil_state =
        {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .rasterizer_state =
        {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        }
    };
    info[GRAPHICS_HIGHLIGHT] = (SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = load_shader(device, "highlight.vert"),
        .fragment_shader = load_shader(device, "highlight.frag"),
        .target_info =
        {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                .blend_state =
                {
                    .enable_blend = true,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
                }
            }},
            .has_depth_stencil_target = true,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        },
        .vertex_input_state =
        {
            .num_vertex_attributes = 3,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {{
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 0,
                .offset = sizeof(float) * 0,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .location = 1,
                .offset = sizeof(float) * 3,
                .buffer_slot = 0,
            },
            {
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 2,
                .offset = sizeof(float) * 5,
                .buffer_slot = 0,
            }},
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {{
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .pitch = sizeof(float) * 8,
                .instance_step_rate = 0,
                .slot = 0,
            }},
        },
        .depth_stencil_state =
        {
            .enable_depth_test = true,
            .enable_depth_write = false,
            .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
        }
    };
    info[GRAPHICS_LIGHT] = (SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = load_shader(device, "fullscreen_flip.vert"),
        .fragment_shader = load_shader(device, "light.frag"),
        .target_info =
        {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT,
            }},
        },
    };
    info[GRAPHICS_COMPOSITE] = (SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = load_shader(device, "fullscreen_flip.vert"),
        .fragment_shader = load_shader(device, "composite.frag"),
        .target_info =
        {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            }},
        },
    };
    computes[COMPUTE_SAMPLER] = load_compute_pipeline(device, "sampler.comp");
    bool status = true;
    for (int i = 0; i < GRAPHICS_COUNT; i++)
    {
        if (info[i].fragment_shader && info[i].vertex_shader)
        {
            graphics[i] = SDL_CreateGPUGraphicsPipeline(device, &info[i]);
            if (!graphics[i])
            {
                SDL_Log("Failed to create pipeline: %s", SDL_GetError());
                status = false;
            }
        }
        else
        {
            status = false;
        }
        if (info[i].fragment_shader)
        {
            SDL_ReleaseGPUShader(device, info[i].fragment_shader);
        }
        if (info[i].vertex_shader)
        {
            SDL_ReleaseGPUShader(device, info[i].vertex_shader);
        }
    }
    for (int i = 0; i < COMPUTE_COUNT; i++)
    {
        if (!computes[i])
        {
            status = false;
            break;
        }
    }
    return status;
}

static bool create_textures()
{
    SDL_GPUTextureCreateInfo info[TEXTURE_COUNT] = {0};
    info[TEXTURE_COLOR] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = RENDERER_WIDTH,
        .height = RENDERER_HEIGHT,
    };
    info[TEXTURE_DEPTH] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = RENDERER_WIDTH,
        .height = RENDERER_HEIGHT,
    };
    info[TEXTURE_POSITION] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = RENDERER_WIDTH,
        .height = RENDERER_HEIGHT,
    };
    info[TEXTURE_NORMAL] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = RENDERER_WIDTH,
        .height = RENDERER_HEIGHT,
    };
    info[TEXTURE_RAY_DEPTH_FRONT] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = rwidth * RENDERER_RAY_OFFSCREEN,
        .height = rheight * RENDERER_RAY_OFFSCREEN,
    };
    info[TEXTURE_RAY_POSITION_FRONT] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = rwidth * RENDERER_RAY_OFFSCREEN,
        .height = rheight * RENDERER_RAY_OFFSCREEN,
    };
    info[TEXTURE_RAY_DEPTH_BACK] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = rwidth * RENDERER_RAY_OFFSCREEN,
        .height = rheight * RENDERER_RAY_OFFSCREEN,
    };
    info[TEXTURE_RAY_POSITION_BACK] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = rwidth * RENDERER_RAY_OFFSCREEN,
        .height = rheight * RENDERER_RAY_OFFSCREEN,
    };
    info[TEXTURE_SUN_DEPTH] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = RENDERER_SUN_RESOLUTION_X,
        .height = RENDERER_SUN_RESOLUTION_Y,
    };
    info[TEXTURE_LIGHT] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = RENDERER_WIDTH,
        .height = RENDERER_HEIGHT,
    };
    info[TEXTURE_COMPOSITE] = (SDL_GPUTextureCreateInfo)
    {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = RENDERER_WIDTH,
        .height = RENDERER_HEIGHT,
    };
    for (int i = 0; i < TEXTURE_COUNT; i++)
    {
        info[i].type = SDL_GPU_TEXTURETYPE_2D,
        info[i].layer_count_or_depth = 1,
        info[i].num_levels = 1,
        textures[i] = SDL_CreateGPUTexture(device, &info[i]);
        if (!textures[i])
        {
            return false;
        }
    }
    return true;
}

static bool create_samplers()
{
    SDL_GPUSamplerCreateInfo info[SAMPLER_COUNT] = {0};
    info[SAMPLER_NEAREST].min_filter = SDL_GPU_FILTER_NEAREST;
    info[SAMPLER_NEAREST].mag_filter = SDL_GPU_FILTER_NEAREST;
    info[SAMPLER_NEAREST].mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    info[SAMPLER_LINEAR].min_filter = SDL_GPU_FILTER_LINEAR;
    info[SAMPLER_LINEAR].mag_filter = SDL_GPU_FILTER_LINEAR;
    info[SAMPLER_LINEAR].mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    for (int i = 0; i < SAMPLER_COUNT; i++)
    {
        info[i].address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info[i].address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info[i].address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplers[i] = SDL_CreateGPUSampler(device, &info[i]);
        if (!samplers[i])
        {
            return false;
        }
    }
    return true;
}

bool renderer_init(
    SDL_Window* a,
    SDL_GPUDevice* b)
{
    assert(a);
    window = a;
    device = b;
    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to create swapchain: %s", SDL_GetError());
        renderer_free();
        return false;
    }
    camera_init(
        &camera,
        CAMERA_TYPE_PERSPECTIVE,
        150.0f,
        RENDERER_WIDTH,
        RENDERER_HEIGHT,
        rad(-45.0f),
        0.0f,
        0.05f);
    float x1;
    float z1;
    float x2;
    float z2;
    camera_update(&camera);
    camera_get_bounds(&camera, &x1, &z1, &x2, &z2);
    rwidth = x2 - x1;
    rheight = z2 - z1;
    camera_init(
        &ray_camera,
        CAMERA_TYPE_ORTHO_3D,
        0.0f,
        rwidth * RENDERER_RAY_OFFSCREEN,
        rheight * RENDERER_RAY_OFFSCREEN,
        rad(-89.9f),
        0.0f,
        1.0f);
    camera_init(
        &sun_camera,
        CAMERA_TYPE_ORTHO_3D,
        0.0f,
        RENDERER_WIDTH * RENDERER_SUN_OFFSCREEN,
        RENDERER_HEIGHT * RENDERER_SUN_OFFSCREEN,
        rad(-45.0f),
        rad(-10.0f),
        1.0f);
    if (!create_pipelines())
    {
        SDL_Log("Failed to create pipelines: %s", SDL_GetError());
        renderer_free();
        return false;
    }
    if (!create_textures())
    {
        SDL_Log("Failed to create textures: %s", SDL_GetError());
        renderer_free();
        return false;
    }
    if (!create_samplers())
    {
        SDL_Log("Failed to create samplers: %s", SDL_GetError());
        renderer_free();
        return false;
    }
    if (!model_init(device))
    {
        SDL_Log("Failed to initialize models");
        renderer_free();
        return false;
    }
    SDL_GPUTransferBufferCreateInfo tbci = {0};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    tbci.size = sizeof(float) * 4;
    SDL_GPUBufferCreateInfo bci = {0};
    bci.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
    bci.size = sizeof(float) * 4;
    sampler_tbo = SDL_CreateGPUTransferBuffer(device, &tbci);
    sampler_sbo = SDL_CreateGPUBuffer(device, &bci);
    if (!sampler_tbo || !sampler_sbo)
    {
        SDL_Log("Failed to create buffer(s): %s", SDL_GetError());
        renderer_free();
        return false;
    }
    return true;
}

void renderer_free()
{
    model_free(device);
    if (sampler_tbo)
    {
        SDL_ReleaseGPUTransferBuffer(device, sampler_tbo);
        sampler_tbo = NULL;
    }
    if (sampler_sbo)
    {
        SDL_ReleaseGPUBuffer(device, sampler_sbo);
        sampler_sbo = NULL;
    }
    for (int i = 0; i < GRAPHICS_COUNT; i++)
    {
        if (graphics[i])
        {
            SDL_ReleaseGPUGraphicsPipeline(device, graphics[i]);
            graphics[i] = NULL;
        }
    }
    for (int i = 0; i < COMPUTE_COUNT; i++)
    {
        if (computes[i])
        {
            SDL_ReleaseGPUComputePipeline(device, computes[i]);
            computes[i] = NULL;
        }
    }
    for (int i = 0; i < TEXTURE_COUNT; i++)
    {
        if (textures[i])
        {
            SDL_ReleaseGPUTexture(device, textures[i]);
            textures[i] = NULL;
        }
    }
    for (int i = 0; i < SAMPLER_COUNT; i++)
    {
        if (samplers[i])
        {
            SDL_ReleaseGPUSampler(device, samplers[i]);
            samplers[i] = NULL;
        }
    }
    SDL_ReleaseWindowFromGPUDevice(device, window);
    window = NULL;
    device = NULL;
}

void renderer_update(
    const float x,
    const float z)
{
    camera_set_target(&camera, x, z);
    camera_update(&camera);
    float x1;
    float z1;
    float x2;
    float z2;
    camera_update(&camera);
    camera_get_bounds(&camera, &x1, &z1, &x2, &z2);
    float a = (x1 + x2) / 2.0f;
    float b = (z1 + z2) / 2.0f;
    a = (int) a / MODEL_SIZE;
    b = (int) b / MODEL_SIZE;
    a *= MODEL_SIZE;
    b *= MODEL_SIZE;
    camera_set_target(&ray_camera, a, b);
    camera_set_target(&sun_camera, a, b);
    camera_update(&ray_camera);
    camera_update(&sun_camera);
}

void renderer_draw()
{
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    {
        SDL_PushGPUDebugGroup(commands, "model");
        SDL_GPUColorTargetInfo cti[3] = {0};
        cti[0].load_op = SDL_GPU_LOADOP_DONT_CARE;
        cti[0].store_op = SDL_GPU_STOREOP_STORE;
        cti[0].texture = textures[TEXTURE_COLOR];
        cti[0].cycle = true;
        cti[1].load_op = SDL_GPU_LOADOP_DONT_CARE;
        cti[1].store_op = SDL_GPU_STOREOP_STORE;
        cti[1].texture = textures[TEXTURE_POSITION];
        cti[1].cycle = true;
        cti[2].load_op = SDL_GPU_LOADOP_DONT_CARE;
        cti[2].store_op = SDL_GPU_STOREOP_STORE;
        cti[2].texture = textures[TEXTURE_NORMAL];
        cti[2].cycle = true;
        SDL_GPUDepthStencilTargetInfo dsti = {0};
        dsti.clear_depth = 1.0f;
        dsti.load_op = SDL_GPU_LOADOP_CLEAR;
        dsti.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        dsti.store_op = SDL_GPU_STOREOP_STORE;
        dsti.texture = textures[TEXTURE_DEPTH];
        dsti.cycle = true;
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, cti, 3, &dsti);
        if (!pass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            goto error;
        }
        SDL_BindGPUGraphicsPipeline(pass, graphics[GRAPHICS_MODEL]);
        SDL_PushGPUVertexUniformData(commands, 0, camera.matrix, 64);
        world_draw_models(device, pass, samplers[SAMPLER_NEAREST]);
        SDL_EndGPURenderPass(pass);
        SDL_PopGPUDebugGroup(commands);
    }
    goto success;
error:
    SDL_PopGPUDebugGroup(commands);
success:
    SDL_SubmitGPUCommandBuffer(commands);
}

void renderer_pick(
    float* x,
    float* y,
    float* z)
{
    assert(x);
    assert(y);
    assert(z);
    if (*x < bx || *x > bx + bwidth || *y < by || *y > by + bheight)
    {
        *x = INFINITY;
        *y = INFINITY;
        return;
    }
    *x = (*x - bx) / bwidth * RENDERER_WIDTH;
    *y = (*y - by) / bheight * RENDERER_HEIGHT;
    *x /= RENDERER_WIDTH;
    *y /= RENDERER_HEIGHT;
    const float uv[2] = { *x, *y };
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    SDL_PushGPUDebugGroup(commands, "pick");
    SDL_GPUStorageBufferReadWriteBinding sbb = {0};
    sbb.buffer = sampler_sbo;
    sbb.cycle = true;
    SDL_GPUComputePass* pass = SDL_BeginGPUComputePass(commands, NULL, 0, &sbb, 1);
    if (!pass)
    {
        SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
        SDL_PopGPUDebugGroup(commands);
        SDL_CancelGPUCommandBuffer(commands);
        return;
    }
    SDL_GPUTextureSamplerBinding tsb = {0};
    tsb.sampler = samplers[SAMPLER_NEAREST];
    tsb.texture = textures[TEXTURE_POSITION];
    SDL_BindGPUComputePipeline(pass, computes[COMPUTE_SAMPLER]);
    SDL_BindGPUComputeSamplers(pass, 0, &tsb, 1);
    SDL_PushGPUComputeUniformData(commands, 0, uv, sizeof(uv));
    SDL_DispatchGPUCompute(pass, 1, 1, 1);
    SDL_EndGPUComputePass(pass);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commands);
    if (!copy)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        SDL_PopGPUDebugGroup(commands);
        SDL_CancelGPUCommandBuffer(commands);
        return;
    }
    SDL_GPUTransferBufferLocation location = {0};
    SDL_GPUBufferRegion region = {0};
    location.transfer_buffer = sampler_tbo;
    region.buffer = sampler_sbo;
    region.size = sizeof(float) * 4;
    SDL_DownloadFromGPUBuffer(copy, &region, &location);
    SDL_EndGPUCopyPass(copy);
    SDL_PopGPUDebugGroup(commands);
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    if (!fence)
    {
        SDL_Log("Failed to acquire fence: %s", SDL_GetError());
        return;
    }
    SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);
    float* data = SDL_MapGPUTransferBuffer(device, sampler_tbo, true);
    if (!data)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return;
    }
    *x = data[0];
    *y = data[1];
    *z = data[2];
    SDL_UnmapGPUTransferBuffer(device, sampler_tbo);
}

void renderer_highlight(
    const model_t model,
    const float x,
    const float y,
    const float z)
{
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    {
        SDL_PushGPUDebugGroup(commands, "highlight");
        SDL_GPUColorTargetInfo cti = {0};
        cti.load_op = SDL_GPU_LOADOP_LOAD;
        cti.store_op = SDL_GPU_STOREOP_STORE;
        cti.texture = textures[TEXTURE_COMPOSITE];
        SDL_GPUDepthStencilTargetInfo dsti = {0};
        dsti.load_op = SDL_GPU_LOADOP_LOAD;
        dsti.store_op = SDL_GPU_STOREOP_DONT_CARE;
        dsti.texture = textures[TEXTURE_DEPTH];
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &cti, 1, &dsti);
        if (!pass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            goto error;
        }
        const float instance[3] = { x, y, z };
        SDL_GPUBufferBinding vbb = {0};
        SDL_GPUBufferBinding ibb = {0};
        vbb.buffer = model_get_vbo(model);
        ibb.buffer = model_get_ibo(model);
        SDL_BindGPUGraphicsPipeline(pass, graphics[GRAPHICS_HIGHLIGHT]);
        SDL_PushGPUVertexUniformData(commands, 0, camera.matrix, 64);
        SDL_PushGPUVertexUniformData(commands, 1, instance, sizeof(instance));
        SDL_BindGPUVertexBuffers(pass, 0, &vbb, 1);
        SDL_BindGPUIndexBuffer(pass, &ibb, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(pass, model_get_num_indices(model), 1, 0, 0, 0);
        SDL_EndGPURenderPass(pass);
        SDL_PopGPUDebugGroup(commands);
    }
    goto success;
error:
    SDL_PopGPUDebugGroup(commands);
success:
    SDL_SubmitGPUCommandBuffer(commands);
}

void renderer_composite()
{
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    {
        SDL_PushGPUDebugGroup(commands, "ray_model_front");
        SDL_GPUColorTargetInfo cti = {0};
        cti.load_op = SDL_GPU_LOADOP_DONT_CARE;
        cti.store_op = SDL_GPU_STOREOP_STORE;
        cti.texture = textures[TEXTURE_RAY_POSITION_FRONT];
        cti.cycle = true;
        SDL_GPUDepthStencilTargetInfo dsti = {0};
        dsti.clear_depth = 1.0f;
        dsti.load_op = SDL_GPU_LOADOP_CLEAR;
        dsti.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        dsti.store_op = SDL_GPU_STOREOP_STORE;
        dsti.texture = textures[TEXTURE_RAY_DEPTH_FRONT];
        dsti.cycle = true;
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &cti, 1, &dsti);
        if (!pass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            goto error;
        }
        SDL_BindGPUGraphicsPipeline(pass, graphics[GRAPHICS_RAY_MODEL_FRONT]);
        SDL_PushGPUVertexUniformData(commands, 0, ray_camera.matrix, 64);
        world_draw_models(device, pass, NULL);
        SDL_EndGPURenderPass(pass);
        SDL_PopGPUDebugGroup(commands);
    }
    {
        SDL_PushGPUDebugGroup(commands, "ray_model_back");
        SDL_GPUColorTargetInfo cti = {0};
        cti.load_op = SDL_GPU_LOADOP_DONT_CARE;
        cti.store_op = SDL_GPU_STOREOP_STORE;
        cti.texture = textures[TEXTURE_RAY_POSITION_BACK];
        cti.cycle = true;
        SDL_GPUDepthStencilTargetInfo dsti = {0};
        dsti.clear_depth = 1.0f;
        dsti.load_op = SDL_GPU_LOADOP_CLEAR;
        dsti.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        dsti.store_op = SDL_GPU_STOREOP_STORE;
        dsti.texture = textures[TEXTURE_RAY_DEPTH_BACK];
        dsti.cycle = true;
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &cti, 1, &dsti);
        if (!pass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            goto error;
        }
        SDL_BindGPUGraphicsPipeline(pass, graphics[GRAPHICS_RAY_MODEL_BACK]);
        SDL_PushGPUVertexUniformData(commands, 0, ray_camera.matrix, 64);
        world_draw_models(device, pass, NULL);
        SDL_EndGPURenderPass(pass);
        SDL_PopGPUDebugGroup(commands);
    }
    {
        SDL_PushGPUDebugGroup(commands, "sun_model");
        SDL_GPUDepthStencilTargetInfo dsti = {0};
        dsti.clear_depth = 1.0f;
        dsti.load_op = SDL_GPU_LOADOP_CLEAR;
        dsti.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        dsti.store_op = SDL_GPU_STOREOP_STORE;
        dsti.texture = textures[TEXTURE_SUN_DEPTH];
        dsti.cycle = true;
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, NULL, 0, &dsti);
        if (!pass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            goto error;
        }
        SDL_BindGPUGraphicsPipeline(pass, graphics[GRAPHICS_SUN_MODEL]);
        SDL_PushGPUVertexUniformData(commands, 0, sun_camera.matrix, 64);
        world_draw_models(device, pass, NULL);
        SDL_EndGPURenderPass(pass);
        SDL_PopGPUDebugGroup(commands);
    }
    {
        SDL_PushGPUDebugGroup(commands, "light");
        SDL_GPUColorTargetInfo cti = {0};
        cti.load_op = SDL_GPU_LOADOP_CLEAR;
        cti.store_op = SDL_GPU_STOREOP_STORE;
        cti.texture = textures[TEXTURE_LIGHT];
        cti.cycle = true;
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &cti, 1, NULL);
        if (!pass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            goto error;
        }
        float sun[3];
        camera_get_vector(&sun_camera, &sun[0], &sun[1], &sun[2]);
        SDL_GPUTextureSamplerBinding tsb[5] = {0};
        tsb[0].sampler = samplers[SAMPLER_NEAREST];
        tsb[0].texture = textures[TEXTURE_POSITION];
        tsb[1].sampler = samplers[SAMPLER_NEAREST];
        tsb[1].texture = textures[TEXTURE_NORMAL];
        tsb[2].sampler = samplers[SAMPLER_LINEAR];
        tsb[2].texture = textures[TEXTURE_RAY_POSITION_FRONT];
        tsb[3].sampler = samplers[SAMPLER_LINEAR];
        tsb[3].texture = textures[TEXTURE_RAY_POSITION_BACK];
        tsb[4].sampler = samplers[SAMPLER_NEAREST];
        tsb[4].texture = textures[TEXTURE_SUN_DEPTH];
        SDL_BindGPUGraphicsPipeline(pass, graphics[GRAPHICS_LIGHT]);
        SDL_BindGPUFragmentSamplers(pass, 0, tsb, 5);
        SDL_PushGPUFragmentUniformData(commands, 0, ray_camera.matrix, 64);
        SDL_PushGPUFragmentUniformData(commands, 1, sun_camera.matrix, 64);
        SDL_PushGPUFragmentUniformData(commands, 2, sun, sizeof(sun));
        world_draw_lights(device, commands, pass);
        SDL_EndGPURenderPass(pass);
        SDL_PopGPUDebugGroup(commands);
    }
    {
        SDL_PushGPUDebugGroup(commands, "composite");
        SDL_GPUColorTargetInfo cti = {0};
        cti.load_op = SDL_GPU_LOADOP_DONT_CARE;
        cti.store_op = SDL_GPU_STOREOP_STORE;
        cti.texture = textures[TEXTURE_COMPOSITE];
        cti.cycle = true;
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &cti, 1, NULL);
        if (!pass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            goto error;
        }
        SDL_GPUTextureSamplerBinding tsb[4] = {0};
        tsb[0].sampler = samplers[SAMPLER_NEAREST];
        tsb[0].texture = textures[TEXTURE_COLOR];
        tsb[1].sampler = samplers[SAMPLER_NEAREST];
        tsb[1].texture = textures[TEXTURE_POSITION];
        tsb[2].sampler = samplers[SAMPLER_NEAREST];
        tsb[2].texture = textures[TEXTURE_NORMAL];
        tsb[3].sampler = samplers[SAMPLER_NEAREST];
        tsb[3].texture = textures[TEXTURE_LIGHT];
        SDL_BindGPUGraphicsPipeline(pass, graphics[GRAPHICS_COMPOSITE]);
        SDL_BindGPUFragmentSamplers(pass, 0, tsb, 4);
        SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);
        SDL_EndGPURenderPass(pass);
        SDL_PopGPUDebugGroup(commands);
    }
    goto success;
error:
    SDL_PopGPUDebugGroup(commands);
success:
    SDL_SubmitGPUCommandBuffer(commands);
}

void renderer_blit()
{
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* swapchain;
    if (!SDL_AcquireGPUSwapchainTexture(commands, window, &swapchain, &width, &height))
    {
        SDL_Log("Failed to aqcuire swapchain image: %s", SDL_GetError());
        goto error;
    }
    if (!commands || width == 0 || height == 0)
    {
        return;
    }
    {
        SDL_PushGPUDebugGroup(commands, "blit");
        const float src = (float) RENDERER_WIDTH / (float) RENDERER_HEIGHT;
        const float dst = (float) width / (float) height;
        float scale;
        if (dst > src)
        {
            scale = (float) height / (float) RENDERER_HEIGHT;
        }
        else
        {
            scale = (float) width / (float) RENDERER_WIDTH;
        }
        bwidth = RENDERER_WIDTH * scale;
        bheight = RENDERER_HEIGHT * scale;
        bx = ((float) width - (float) bwidth) / 2.0f;
        by = ((float) height - (float) bheight) / 2.0f;
        SDL_GPUBlitInfo blit = {0};
        blit.source.x = 0;
        blit.source.y = 0;
        blit.source.w = RENDERER_WIDTH;
        blit.source.h = RENDERER_HEIGHT;
        blit.source.texture = textures[TEXTURE_COMPOSITE];
        blit.destination.x = bx;
        blit.destination.y = by;
        blit.destination.w = bwidth;
        blit.destination.h = bheight;
        blit.destination.texture = swapchain;
        blit.filter = SDL_GPU_FILTER_NEAREST;
        SDL_BlitGPUTexture(commands, &blit);
        SDL_PopGPUDebugGroup(commands);
    }
    goto success;
error:
    SDL_PopGPUDebugGroup(commands);
success:
    SDL_SubmitGPUCommandBuffer(commands);
}

void renderer_get_bounds(
    float* x1,
    float* z1,
    float* x2,
    float* z2)
{
    assert(x1);
    assert(z1);
    assert(x2);
    assert(z2);
    *x1 = ray_camera.x - ray_camera.width;
    *z1 = ray_camera.z - ray_camera.height;
    *x2 = ray_camera.x + ray_camera.width;
    *z2 = ray_camera.z + ray_camera.height;
}