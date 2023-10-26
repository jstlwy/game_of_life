#include "graphics.h"
#include <stdlib.h>
#include <stdio.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define NUM_PIXELS    (SCREEN_WIDTH * SCREEN_HEIGHT)
#define NUM_BYTES     (NUM_PIXELS * sizeof(uint32_t))
#define TEXTURE_PITCH (SCREEN_WIDTH * sizeof(uint32_t))

void init_graphics(graphics_t* const gfx)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    
    SDL_Window* const window = SDL_CreateWindow(
        "Ray Tracing Project",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (window == NULL) {
        fprintf(stderr, "Window error: %s\n", SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    
    SDL_Renderer* const renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        fprintf(stderr, "Renderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(gfx->window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    
    SDL_Texture* const texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    if (texture == NULL) {
        fprintf(stderr, "Error when creating texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(gfx->renderer);
        SDL_DestroyWindow(gfx->window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    uint32_t* const pixels = malloc(NUM_PIXELS * sizeof(uint32_t));
    if (pixels == NULL) {
        fprintf(stderr, "Failed to allocate memory for the pixel array.\n");
        SDL_DestroyTexture(gfx->texture);
        SDL_DestroyRenderer(gfx->renderer);
        SDL_DestroyWindow(gfx->window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    
    gfx->window = window;
    gfx->renderer = renderer;
    gfx->texture = texture;
    gfx->pixels = pixels;
}

void render_graphics(graphics_t* const gfx)
{
    //SDL_RenderClear(gfx->renderer);
    SDL_UpdateTexture(gfx->texture, NULL, gfx->pixels, TEXTURE_PITCH);
    SDL_RenderCopy(gfx->renderer, gfx->texture, NULL, NULL);
    SDL_RenderPresent(gfx->renderer);
}

void end_graphics(graphics_t* const gfx)
{
    free(gfx->pixels);
    SDL_DestroyTexture(gfx->texture);
    SDL_DestroyRenderer(gfx->renderer);
    SDL_DestroyWindow(gfx->window);
    SDL_Quit();
}

