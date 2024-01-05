#include "graphics.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

bool was_initialized = false;

struct sdl_graphics init_graphics(const char* const title)
{
    if (was_initialized) {
        fprintf(stderr, "Only one instance of sdl_graphics may exist at a time.\n");
        exit(EXIT_FAILURE);
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    
    SDL_Window* const window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (window == NULL) {
        fprintf(stderr, "Window error: %s\n", SDL_GetError());
        goto quit_sdl;
    }
    
    SDL_Renderer* const renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        fprintf(stderr, "Renderer error: %s\n", SDL_GetError());
        goto destroy_window;
    }
    
    SDL_Texture* const texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    if (texture != NULL) {
        was_initialized = true;
        struct sdl_graphics gfx = {
            .window = window,
            .renderer = renderer,
            .texture = texture,
        };
        return gfx;
    }

    fprintf(stderr, "Error when creating texture: %s\n", SDL_GetError());

    SDL_DestroyRenderer(renderer);
destroy_window:
    SDL_DestroyWindow(window);
quit_sdl:
    SDL_Quit();
    exit(EXIT_FAILURE);
}

void render_graphics(struct sdl_graphics* const gfx, const uint32_t pixels[const static NUM_PIXELS])
{
    //SDL_RenderClear(gfx->renderer);
    SDL_UpdateTexture(gfx->texture, NULL, pixels, TEXTURE_PITCH);
    SDL_RenderCopy(gfx->renderer, gfx->texture, NULL, NULL);
    SDL_RenderPresent(gfx->renderer);
}

void end_graphics(struct sdl_graphics* const gfx)
{
    SDL_DestroyTexture(gfx->texture);
    SDL_DestroyRenderer(gfx->renderer);
    SDL_DestroyWindow(gfx->window);
    SDL_Quit();
    was_initialized = false;
}

