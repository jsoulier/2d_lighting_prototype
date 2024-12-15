#pragma once

#include <stdbool.h>
#include "model.h"

void world_free(
    SDL_GPUDevice* device);
void world_update(
    SDL_GPUDevice* device,
    const float x1,
    const float z1,
    const float x2,
    const float z2);
void world_draw_models(
    SDL_GPUDevice* device,
    SDL_GPURenderPass* pass,
    SDL_GPUSampler* sampler);
void world_draw_lights(
    SDL_GPUDevice* device,
    SDL_GPUCommandBuffer* commands,
    SDL_GPURenderPass* pass);
void world_set_model(
    const model_t model,
    const int x,
    const int z);
model_t world_get_model(
    const int x,
    const int z);