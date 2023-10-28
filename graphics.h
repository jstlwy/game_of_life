#ifndef graphics_h
#define graphics_h

#include <stdint.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define NUM_PIXELS    (SCREEN_WIDTH * SCREEN_HEIGHT)
#define NUM_BYTES     (NUM_PIXELS * sizeof(uint32_t))
#define TEXTURE_PITCH (SCREEN_WIDTH * sizeof(uint32_t))

struct sdl_graphics {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint32_t* pixels;
} ;

void init_graphics(struct sdl_graphics* const gfx);
void render_graphics(struct sdl_graphics* const gfx);
void end_graphics(struct sdl_graphics* const gfx);

#endif

