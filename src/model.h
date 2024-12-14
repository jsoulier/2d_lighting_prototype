#pragma once

#include <SDL3/SDL.h>

#define MODELS \
    X(DIRT, 0) \
    X(GRASS, 0) \
    X(LAVA, 50) \
    X(LIGHTHOUSE, 100) \
    X(ROCK1, 0) \
    X(ROCK2, 0) \
    X(ROCK3, 0) \
    X(ROCK4, 0) \
    X(ROCK5, 0) \
    X(SAND, 0) \
    X(TREE1, 0) \
    X(TREE2, 0) \
    X(TREE3, 0) \
    X(WATER, 0) \

typedef enum
{
#define X(name, illuminance) MODEL_##name,
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
int model_get_illuminance(
    const model_t model);
const char* model_get_name(
    const model_t model);