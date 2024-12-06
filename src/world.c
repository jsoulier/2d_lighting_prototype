#include <SDL3/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "database.h"
#include "helpers.h"
#include "model.h"
#include "world.h"

static model_t* models;
static SDL_GPUTransferBuffer* tbos[MODEL_COUNT];
static SDL_GPUBuffer* vbos[MODEL_COUNT];
static int max_instances[MODEL_COUNT];
static int instances[MODEL_COUNT];
static SDL_GPUTransferBuffer* light_tbo;
static SDL_GPUBuffer* light_sbo;
static uint32_t lights;
static int max_lights;
static int wwidth;
static int wheight;
static int wx;
static int wz;
static bool dirty;

model_t world_get_model(
    int x,
    int z)
{
    const int a = x - wx;
    const int b = z - wz;
    if (a < 0 || b < 0 || a >= wwidth || b >= wheight)
    {
        SDL_Log("Model out of bounds");
        return MODEL_COUNT;
    }
    return models[b * wwidth + a];
}

static void set_model(
    const model_t model,
    int x,
    int z)
{
    const int a = x - wx;
    const int b = z - wz;
    if (a < 0 || b < 0 || a >= wwidth || b >= wheight)
    {
        SDL_Log("Model out of bounds");
        return;
    }
    models[b * wwidth + a] = model;
}

void world_free(
    SDL_GPUDevice* device)
{
    free(models);
    for (model_t model = 0; model < MODEL_COUNT; model++)
    {
        if (tbos[model])
        {
            SDL_ReleaseGPUTransferBuffer(device, tbos[model]);
            tbos[model] = NULL;
        }
        if (vbos[model])
        {
            SDL_ReleaseGPUBuffer(device, vbos[model]);
            vbos[model] = NULL;
        }
    }
    if (light_tbo)
    {
        SDL_ReleaseGPUTransferBuffer(device, light_tbo);
        light_tbo = NULL;
    }
    if (light_sbo)
    {
        SDL_ReleaseGPUBuffer(device, light_sbo);
        light_sbo = NULL;
    }
    memset(instances, 0, sizeof(instances));
    memset(max_instances, 0, sizeof(max_instances));
    models = NULL;
    device = NULL;
}

void world_update(
    SDL_GPUDevice* device,
    const float x1,
    const float z1,
    const float x2,
    const float z2)
{
    const int sx = floorf(x1 / MODEL_SIZE);
    const int sz = floorf(z1 / MODEL_SIZE);
    const int ex = ceilf(x2 / MODEL_SIZE);
    const int ez = ceilf(z2 / MODEL_SIZE);
    if (!dirty && sx == wx && sz == wz)
    {
        return;
    }
    const int nwidth = ex - sx;
    const int nheight = ez - sz;
    if (nwidth != wwidth || nheight != wheight)
    {
        free(models);
        models = malloc(nwidth * nheight * sizeof(model_t));
        if (!models)
        {
            SDL_Log("Failed to allocate models");
            return;
        }
        wwidth = nwidth;
        wheight = nheight;
    }
    wx = sx;
    wz = sz;
    lights = 0;
    memset(instances, 0, sizeof(instances));
    memset(models, 0, wwidth * wheight * sizeof(model_t));
    database_get_models(set_model, sx, sz, ex, ez);
    for (int x = sx; x < ex; x++)
    {
        for (int z = sz; z < ez; z++)
        {
            const model_t model = world_get_model(x, z);
            instances[model]++;
            lights += model_get_illuminance(model) > 0;
        }
    }
    float* mdata[MODEL_COUNT] = {0};
    float* ldata = NULL;
    for (model_t model = 0; model < MODEL_COUNT; model++)
    {
        if (!instances[model])
        {
            continue;
        }
        if (instances[model] > max_instances[model])
        {
            max_instances[model] = 0;
            if (tbos[model])
            {
                SDL_ReleaseGPUTransferBuffer(device, tbos[model]);
                tbos[model] = NULL;
            }
            if (vbos[model])
            {
                SDL_ReleaseGPUBuffer(device, vbos[model]);
                vbos[model] = NULL;
            }
            SDL_GPUTransferBufferCreateInfo tbci = {0};
            tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            tbci.size = instances[model] * sizeof(float) * 4;
            SDL_GPUBufferCreateInfo bci = {0};
            bci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            bci.size = instances[model] * sizeof(float) * 4;
            tbos[model] = SDL_CreateGPUTransferBuffer(device, &tbci);
            vbos[model] = SDL_CreateGPUBuffer(device, &bci);
            if (!tbos[model] || !vbos[model])
            {
                SDL_Log("Failed to create buffer(s): %s", SDL_GetError());
                return;
            }
            max_instances[model] = instances[model];
        }
        mdata[model] = SDL_MapGPUTransferBuffer(device, tbos[model], true);
        if (!mdata[model])
        {
            SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
            return;
        }
        instances[model] = 0;
    }
    if (lights > max_lights)
    {
        max_lights = 0;
        if (light_tbo)
        {
            SDL_ReleaseGPUTransferBuffer(device, light_tbo);
            light_tbo = NULL;
        }
        if (light_sbo)
        {
            SDL_ReleaseGPUBuffer(device, light_sbo);
            light_sbo = NULL;
        }
        SDL_GPUTransferBufferCreateInfo tbci = {0};
        tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbci.size = lights * sizeof(float) * 4;
        SDL_GPUBufferCreateInfo bci = {0};
        bci.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        bci.size = lights * sizeof(float) * 4;
        light_tbo = SDL_CreateGPUTransferBuffer(device, &tbci);
        light_sbo = SDL_CreateGPUBuffer(device, &bci);
        if (!light_tbo || !light_sbo)
        {
            SDL_Log("Failed to create buffer(s): %s", SDL_GetError());
            return;
        }
        max_lights = lights;
    }
    if (lights)
    {
        ldata = SDL_MapGPUTransferBuffer(device, light_tbo, true);
        if (!ldata)
        {
            SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
            return;
        }
    }
    lights = 0;
    for (int x = sx; x < ex; x++)
    {
        for (int z = sz; z < ez; z++)
        {
            const model_t model = world_get_model(x, z);
            mdata[model][instances[model] * 4 + 0] = x * MODEL_SIZE;
            mdata[model][instances[model] * 4 + 1] = 0.0f;
            mdata[model][instances[model] * 4 + 2] = z * MODEL_SIZE;
            mdata[model][instances[model] * 4 + 3] = 0.0f;
            instances[model]++;
            if (model_get_illuminance(model) <= 0)
            {
                continue;
            }
            ldata[lights * 4 + 0] = x * MODEL_SIZE;
            ldata[lights * 4 + 1] = model_get_height(model);
            ldata[lights * 4 + 2] = z * MODEL_SIZE;
            ldata[lights * 4 + 3] = model_get_illuminance(model);
            lights++;
        }
    }
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commands);
    if (!copy)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        return;
    }
    for (model_t model = 0; model < MODEL_COUNT; model++)
    {
        if (!mdata[model])
        {
            continue;
        }
        SDL_UnmapGPUTransferBuffer(device, tbos[model]);
        SDL_GPUTransferBufferLocation location = {0};
        SDL_GPUBufferRegion region = {0};
        location.transfer_buffer = tbos[model];
        region.buffer = vbos[model];
        region.size = instances[model] * sizeof(float) * 4;
        SDL_UploadToGPUBuffer(copy, &location, &region, true);
    }
    if (lights)
    {
        SDL_UnmapGPUTransferBuffer(device, light_tbo);
        SDL_GPUTransferBufferLocation location = {0};
        SDL_GPUBufferRegion region = {0};
        location.transfer_buffer = light_tbo;
        region.buffer = light_sbo;
        region.size = lights * sizeof(float) * 4;
        SDL_UploadToGPUBuffer(copy, &location, &region, true);
    }
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(commands);
    dirty = false;
}

void world_render(
    SDL_GPUDevice* device,
    SDL_GPURenderPass* pass,
    SDL_GPUSampler* sampler)
{
    assert(device);
    assert(pass);
    for (model_t model = 0; model < MODEL_COUNT; model++)
    {
        if (!instances[model])
        {
            continue;
        }
        SDL_GPUBufferBinding vbb[2] = {0};
        SDL_GPUBufferBinding ibb = {0};
        vbb[0].buffer = model_get_vbo(model);
        vbb[1].buffer = vbos[model];
        ibb.buffer = model_get_ibo(model);
        SDL_BindGPUVertexBuffers(pass, 0, vbb, 2);
        SDL_BindGPUIndexBuffer(pass, &ibb, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        if (sampler)
        {
            SDL_GPUTextureSamplerBinding tsb = {0};
            tsb.sampler = sampler;
            tsb.texture = model_get_palette(model);
            SDL_BindGPUFragmentSamplers(pass, 0, &tsb, 1);
        }
        const int num_indices = model_get_num_indices(model);
        const int count = instances[model];
        SDL_DrawGPUIndexedPrimitives(pass, num_indices, count, 0, 0, 0);
    }
}

void world_render_lights(
    SDL_GPUDevice* device,
    SDL_GPUCommandBuffer* commands,
    SDL_GPURenderPass* pass)
{
    assert(device);
    assert(commands);
    assert(pass);
    if (!lights)
    {
        return;
    }
    SDL_BindGPUFragmentStorageBuffers(pass, 0, &light_sbo, 1);
    SDL_PushGPUFragmentUniformData(commands, 1, &lights, 4);
    SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);
}

void world_set_model(
    const model_t model,
    const int x,
    const int z)
{
    set_model(model, x, z);
    database_set_model(model, x, z);
    dirty = true;
}