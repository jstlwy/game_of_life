#ifndef graphics_h
#define graphics_h

#include <stdint.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define NUM_PIXELS    (SCREEN_WIDTH * SCREEN_HEIGHT)
#define NUM_BYTES     (NUM_PIXELS * sizeof(uint32_t))
#define TEXTURE_PITCH (SCREEN_WIDTH * sizeof(uint32_t))

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint32_t* pixels;
} graphics_t;

void init_graphics(graphics_t* const gfx);
void render_graphics(graphics_t* const gfx);
void end_graphics(graphics_t* const gfx);

#endif

