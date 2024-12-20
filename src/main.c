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

int main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Window* window = window = SDL_CreateWindow("", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
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
    model_t selected;
    float x;
    float z;
    database_get_state(&selected, &x, &z);
    SDL_SetWindowResizable(window, true);
    SDL_SetWindowTitle(window, model_get_str(selected));
    bool running = true;
    uint64_t t1 = SDL_GetPerformanceCounter();
    uint64_t t2 = 0;
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
                selected += event.wheel.y;
                if (selected < 0)
                {
                    selected = MODEL_COUNT - 1;
                }
                else if (selected >= MODEL_COUNT)
                {
                    selected = 0;
                }
                SDL_SetWindowTitle(window, model_get_str(selected));
                break;
            }
        }
        {
            const bool* keys = SDL_GetKeyboardState(NULL);
            float dx = 0.0f;
            float dz = 0.0f;
            if (keys[SDL_SCANCODE_W])
            {
                dz -= SPEED;
            }
            else if (keys[SDL_SCANCODE_S])
            {
                dz += SPEED;
            }
            if (keys[SDL_SCANCODE_D])
            {
                dx += SPEED;
            }
            else if (keys[SDL_SCANCODE_A])
            {
                dx -= SPEED;
            }
            x += dx * dt;
            z += dz * dt;
        }
        {
            float x1;
            float z1;
            float x2;
            float z2;
            renderer_update(x, z);
            renderer_get_bounds(&x1, &z1, &x2, &z2);
            world_update(device, x1, z1, x2, z2);
        }
        renderer_draw();
        renderer_composite();
        {
            float mx;
            float my;
            float mz;
            int width;
            SDL_GetWindowSizeInPixels(window, &width, NULL);
            SDL_GetMouseState(&mx, &my);
            const float sx = mx;
            renderer_get_position(&mx, &my, &mz);
            width /= 2;
            mz -= PICK_BIAS;
            if (sx < width)
            {
                mx -= PICK_BIAS;
            }
            else
            {
                mx += PICK_BIAS;
            }
            if (mx > 0.0f)
            {
                mx = (int) (mx + MODEL_SIZE / 2.0f) / MODEL_SIZE;
            }
            else
            {
                mx = (int) (mx - MODEL_SIZE / 2.0f) / MODEL_SIZE;
            }
            if (mz < 0.0f)
            {
                mz = (int) (mz - MODEL_SIZE / 2.0f) / MODEL_SIZE;
            }
            else
            {
                mz = (int) (mz + MODEL_SIZE / 2.0f) / MODEL_SIZE;
            }
            const int buttons = SDL_GetMouseState(NULL, NULL);
            if (buttons & SDL_BUTTON_RMASK)
            {
                world_set_model(selected, mx, mz);
            }
            else if (buttons & SDL_BUTTON_LMASK)
            {
                world_set_model(0, mx, mz);
            }
            const model_t picked = world_get_model(mx, mz);
            if (picked != MODEL_COUNT)
            {
                mx *= MODEL_SIZE;
                mz *= MODEL_SIZE;
                renderer_highlight(picked, mx, 0.0f, mz);
            }
        }
        renderer_blit();
        database_set_state(selected, x, z);
    }
    world_free(device);
    database_set_state(selected, x, z);
    database_free();
    renderer_free();
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}