#pragma once

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "model.h"

bool renderer_init(
    SDL_Window* window,
    SDL_GPUDevice* device);
void renderer_free();
void renderer_update(
    const float x,
    const float z);
void renderer_begin_frame();
void renderer_get_position(
    float* x,
    float* y,
    float* z);
void renderer_composite();
void renderer_highlight(
    const model_t model,
    const float x,
    const float y,
    const float z);
void renderer_end_frame();
void renderer_get_bounds(
    float* x1,
    float* z1,
    float* x2,
    float* z2);