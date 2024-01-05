#ifndef graphics_h
#define graphics_h

#include <stdint.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH         640
#define SCREEN_HEIGHT        480
#define NUM_PIXELS           (SCREEN_WIDTH * SCREEN_HEIGHT)
#define NUM_BYTES_IN_TEXTURE (NUM_PIXELS * sizeof(uint32_t))
#define TEXTURE_PITCH        (SCREEN_WIDTH * sizeof(uint32_t))

struct sdl_graphics {
    SDL_Window* const window;
    SDL_Renderer* const renderer;
    SDL_Texture* const texture;
};

struct sdl_graphics init_graphics(const char* const title);
void render_graphics(struct sdl_graphics* const gfx, const uint32_t pixels[const static NUM_PIXELS]);
void end_graphics(struct sdl_graphics* const gfx);

#endif

