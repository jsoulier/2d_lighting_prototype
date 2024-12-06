#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "config.h"
#include "database.h"
#include "helpers.h"
#include "renderer.h"
#include "model.h"
#include "world.h"

static SDL_GPUDevice* device;
static SDL_Window* window;
static model_t selection;

static void move(
    float* x,
    float* z,
    const float dt)
{
    const bool* keys = SDL_GetKeyboardState(NULL);
    const float speed = 500.0f;
    float dx = 0.0f;
    float dz = 0.0f;
    if (keys[SDL_SCANCODE_W])
    {
        dz -= speed;
    }
    else if (keys[SDL_SCANCODE_S])
    {
        dz += speed;
    }
    if (keys[SDL_SCANCODE_D])
    {
        dx += speed;
    }
    else if (keys[SDL_SCANCODE_A])
    {
        dx -= speed;
    }
    *x += dx * dt;
    *z += dz * dt;
}

static model_t pick(
    float* x,
    float* z)
{
    float mx;
    float my;
    int size;
    SDL_GetWindowSizeInPixels(window, &size, NULL);
    SDL_GetMouseState(&mx, &my);
    *x = mx;
    size /= 2;
    renderer_get_position(x, &my, z);
    const float bias = 0.01f;
    const float center = MODEL_SIZE / 2.0f;
    *z -= bias;
    if (mx < size)
    {
        *x -= bias;
    }
    else
    {
        *x += bias;
    }
    if (*x > 0.0f)
    {
        *x = (int) (*x + center) / MODEL_SIZE;
    }
    else
    {
        *x = (int) (*x - center) / MODEL_SIZE;
    }
    if (*z < 0.0f)
    {
        *z = (int) (*z - center) / MODEL_SIZE;
    }
    else
    {
        *z = (int) (*z + center) / MODEL_SIZE;
    }
    return world_get_model(*x, *z);
}

int main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    window = SDL_CreateWindow("prototype", RENDERER_WIDTH * 2.0f, RENDERER_HEIGHT * 2.0f, 0);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_SetWindowResizable(window, true);
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }
    if (!renderer_init(window, device))
    {
        SDL_Log("Failed to initialize renderer");
        return EXIT_FAILURE;
    }
    if (!database_init(DATABASE_PATH))
    {
        SDL_Log("Failed to initialize database");
        return EXIT_FAILURE;
    }
    uint64_t t1 = SDL_GetPerformanceCounter();
    uint64_t t2 = 0;
    float x = 0.0f;
    float z = 0.0f;
    database_get_state(&selection, &x, &z);
    float cooldown = 0.0f;
    bool running = true;
    int model_cooldown = 0;
    while (running)
    {
        t2 = t1;
        t1 = SDL_GetPerformanceCounter();
        const float frequency = SDL_GetPerformanceFrequency();
        const float dt = (t1 - t2) / frequency;
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if (!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MMASK))
                {
                    selection += event.wheel.y;
                    if (selection < 0)
                    {
                        selection = MODEL_COUNT - 1;
                    }
                    else if (selection >= MODEL_COUNT)
                    {
                        selection = 0;
                    }
                }
                break;
            }
        }
        move(&x, &z, dt);
        float x1;
        float z1;
        float x2;
        float z2;
        renderer_update(x, z);
        renderer_get_bounds(&x1, &z1, &x2, &z2);
        world_update(device, x1, z1, x2, z2);
        renderer_begin_frame();
        renderer_composite();
        float bx;
        float bz;
        const model_t model = pick(&bx, &bz);
        const int buttons = SDL_GetMouseState(NULL, NULL);
        if (buttons & SDL_BUTTON_LMASK)
        {
            world_set_model(selection, bx, bz);
        }
        if (model != MODEL_COUNT)
        {
            bx *= MODEL_SIZE;
            bz *= MODEL_SIZE;
            renderer_highlight(model, bx, 0.0f, bz);
        }
        renderer_end_frame(&selection);
        cooldown += dt;
        if (cooldown > DATABASE_COOLDOWN && database_commit())
        {
            database_set_state(selection, x, z);
            cooldown = 0.0f;
        }
    }
    world_free(device);
    database_set_state(selection, x, z);
    database_free();
    renderer_free();
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}