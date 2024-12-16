#pragma once

#include <SDL3/SDL.h>

#define MODELS \
    X(GRASS, 0) \
    X(DIRT, 0) \
    X(WATER, 0) \
    X(ROCK1, 0) \
    X(ROCK2, 0) \
    X(ROCK3, 0) \
    X(ROCK4, 0) \
    X(ROCK5, 0) \
    X(SAND, 0) \
    X(TREE1, 0) \
    X(TREE2, 0) \
    X(TREE3, 0) \
    X(LAVA, 50) \
    X(LIGHTHOUSE, 100) \

typedef enum
{
#define X(name, spread) MODEL_##name,
    MODELS
#undef X
    MODEL_COUNT,
}
model_t;

bool model_init(
    SDL_GPUDevice* device);
void model_free(
    SDL_GPUDevice* device);
SDL_GPUBuffer* model_get_vbo(
    const model_t model);
SDL_GPUBuffer* model_get_ibo(
    const model_t model);
SDL_GPUTexture* model_get_palette(
    const model_t model);
int model_get_num_indices(
    const model_t model);
int model_get_height(
    const model_t model);
int model_get_spread(
    const model_t model);
const char* model_get_str(
    const model_t model);