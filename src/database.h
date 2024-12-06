#pragma once

#include <stdbool.h>
#include "model.h"

typedef void (*database_get_models_func_t)(
    const model_t model,
    const int x,
    const int z);

bool database_init(
    const char* path);
void database_free();
bool database_commit();
void database_set_state(
    const model_t model,
    const float x,
    const float z);
void database_get_state(
    model_t* model,
    float* x,
    float* z);
void database_set_model(
    const model_t model,
    const int x,
    const int z);
void database_get_models(
    const database_get_models_func_t func,
    const int x1,
    const int z1,
    const int x2,
    const int z2);