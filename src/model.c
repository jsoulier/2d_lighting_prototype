#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <tinyobj_loader_c.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "helpers.h"
#include "model.h"

typedef struct
{
    float vx;
    float vy;
    float vz;
    float tx;
    float ty;
    float nx;
    float ny;
    float nz;
}
vertex_t;

struct
{
    SDL_GPUBuffer* vbo;
    SDL_GPUBuffer* ibo;
    SDL_GPUTexture* palette;
    int num_indices;
    int height;
    int illuminance;
}
static models[MODEL_COUNT];

static void func(
    void* ctx,
    const char* file,
    int mtl,
    const char* obj,
    char** data,
    size_t* size)
{
    assert(file);
    assert(data);
    assert(size);
    *data = SDL_LoadFile(file, size);
    if (!data)
    {
        SDL_Log("Failed to load model: %s", file);
    }
}

static bool load(
    const model_t model,
    const char* name,
    SDL_GPUDevice* device,
    SDL_GPUCopyPass* pass)
{
    bool status = true;
    tinyobj_attrib_t attrib = {0};
    tinyobj_shape_t* shapes = NULL;
    tinyobj_material_t* materials = NULL;
    size_t num_shapes;
    size_t num_materials;
    char obj[256];
    char png[256];
    snprintf(obj, sizeof(obj), "%s.obj", name);
    snprintf(png, sizeof(png), "%s.png", name);
    if (tinyobj_parse_obj(
        &attrib,
        &shapes,
        &num_shapes,
        &materials,
        &num_materials,
        obj,
        func,
        NULL,
        TINYOBJ_FLAG_TRIANGULATE) != TINYOBJ_SUCCESS)
    {
        SDL_Log("Failed to parse model: %s", name);
        goto error;
    }
    models[model].height = 0;
    models[model].num_indices = attrib.num_faces;
    models[model].palette = load_texture(device, png);
    if (!models[model].palette)
    {
        SDL_Log("Failed to load palette: %s", name);
        goto error;
    }
    struct
    {
        vertex_t key;
        int value;
    }
    *map = NULL;
    stbds_hmdefault(map, -1);
    if (!map)
    {
        SDL_Log("Failed to create map: %ss", name);
        goto error;
    }
    SDL_GPUTransferBufferCreateInfo tbci = {0};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size = attrib.num_faces * sizeof(vertex_t);
    SDL_GPUTransferBuffer* vbo = SDL_CreateGPUTransferBuffer(device, &tbci);
    tbci.size = attrib.num_faces * sizeof(uint32_t);
    SDL_GPUTransferBuffer* ibo = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!vbo || !ibo)
    {
        SDL_Log("Failed to create transfer buffer(s): %s, %s", name, SDL_GetError());
        goto error;
    }
    vertex_t* vertices = SDL_MapGPUTransferBuffer(device, vbo, false);
    uint32_t* indices = SDL_MapGPUTransferBuffer(device, ibo, false);
    if (!vertices || !indices)
    {
        SDL_Log("Failed to map transfer buffer(s): %s, %s", name, SDL_GetError());
        goto error;
    }
    int num_vertices = 0;
    for (int i = 0; i < attrib.num_faces; i++)
    {
        const tinyobj_vertex_index_t tvi = attrib.faces[i];
        if (tvi.v_idx < 0 || tvi.vn_idx < 0 || tvi.vt_idx < 0)
        {
            SDL_Log("Missing model data: %s", name);
            goto error;
        }
        vertex_t vertex;
        vertex.vx = attrib.vertices[3 * tvi.v_idx + 0] * 10.0f;
        vertex.vy = attrib.vertices[3 * tvi.v_idx + 1] * 10.0f;
        vertex.vz = attrib.vertices[3 * tvi.v_idx + 2] * 10.0f;
        vertex.tx = attrib.texcoords[2 * tvi.vt_idx + 0];
        vertex.ty = attrib.texcoords[2 * tvi.vt_idx + 1];
        vertex.nx = attrib.normals[3 * tvi.vn_idx + 0];
        vertex.ny = attrib.normals[3 * tvi.vn_idx + 1];
        vertex.nz = attrib.normals[3 * tvi.vn_idx + 2];
        const int index = stbds_hmget(map, vertex);
        if (index == -1)
        {
            models[model].height = max(models[model].height, vertex.vy);
            stbds_hmput(map, vertex, num_vertices);
            vertices[num_vertices] = vertex;
            indices[i] = num_vertices++;
        }
        else
        {
            indices[i] = index;
        }
    }
    SDL_UnmapGPUTransferBuffer(device, vbo);
    SDL_UnmapGPUTransferBuffer(device, ibo);
    SDL_GPUBufferCreateInfo vbci = {0};
    SDL_GPUBufferCreateInfo ibci = {0};
    vbci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    ibci.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    vbci.size = num_vertices * sizeof(vertex_t);
    ibci.size = models[model].num_indices * sizeof(uint32_t);
    models[model].vbo = SDL_CreateGPUBuffer(device, &vbci);
    models[model].ibo = SDL_CreateGPUBuffer(device, &ibci);
    if (!models[model].vbo || !models[model].ibo)
    {
        SDL_Log("Failed to create model buffer(s): %s, %s", name, SDL_GetError());
        goto error;
    }
    SDL_GPUTransferBufferLocation location = {0};
    SDL_GPUBufferRegion region = {0};
    location.transfer_buffer = vbo;
    region.buffer = models[model].vbo;
    region.size = num_vertices * sizeof(vertex_t);
    SDL_UploadToGPUBuffer(pass, &location, &region, false);
    location.transfer_buffer = ibo;
    region.buffer = models[model].ibo;
    region.size = models[model].num_indices * sizeof(uint32_t);
    SDL_UploadToGPUBuffer(pass, &location, &region, false);
    goto success;
error:
    status = false;
success:
    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    stbds_hmfree(map);
    SDL_ReleaseGPUTransferBuffer(device, vbo);
    SDL_ReleaseGPUTransferBuffer(device, ibo);
    return status;
}

bool model_init(
    SDL_GPUDevice* device)
{
    assert(device);
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return false;
    }
    SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(commands);
    if (!pass)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }
    const char* names[MODEL_COUNT] =
    {
#define X(name, illuminance) #name,
        MODELS
#undef X
    };
    const int illuminances[MODEL_COUNT] =
    {
#define X(name, illuminance) illuminance,
        MODELS
#undef X
    };
    bool status = true;
    for (model_t model = 0; model < MODEL_COUNT; model++)
    {
        const char* src = names[model];
        char dst[256] = {0};
        for (int i = 0; i < 256 && src[i]; i++)
        {
            dst[i] = tolower(src[i]);
        }
        models[model].illuminance = illuminances[model];
        if (!load(model, dst, device, pass))
        {
            SDL_Log("Failed to load model: %s", dst);
            status = false;
            break;
        }
    }
    SDL_EndGPUCopyPass(pass);
    SDL_SubmitGPUCommandBuffer(commands);
    if (!status)
    {
        model_free(device);
    }
    return status;
}

void  model_free(
    SDL_GPUDevice* device)
{
    assert(device);
    for (model_t model = 0; model < MODEL_COUNT; model++)
    {
        if (models[model].vbo)
        {
            SDL_ReleaseGPUBuffer(device, models[model].vbo);
            models[model].vbo = NULL;
        }
        if (models[model].ibo)
        {
            SDL_ReleaseGPUBuffer(device, models[model].ibo);
            models[model].ibo = NULL;
        }
        if (models[model].palette)
        {
            SDL_ReleaseGPUTexture(device, models[model].palette);
            models[model].palette = NULL;
        }
    }
}

SDL_GPUBuffer* model_get_vbo(
    const model_t model)
{
    assert(model < MODEL_COUNT);
    return models[model].vbo;
}

SDL_GPUBuffer* model_get_ibo(
    const model_t model)
{
    assert(model < MODEL_COUNT);
    return models[model].ibo;
}

SDL_GPUTexture* model_get_palette(
    const model_t model)
{
    assert(model < MODEL_COUNT);
    return models[model].palette;
}

int model_get_num_indices(
    const model_t model)
{
    assert(model < MODEL_COUNT);
    return models[model].num_indices;
}

int model_get_height(
    const model_t model)
{
    assert(model < MODEL_COUNT);
    return models[model].height;
}

int model_get_illuminance(
    const model_t model)
{
    assert(model < MODEL_COUNT);
    return models[model].illuminance;
}