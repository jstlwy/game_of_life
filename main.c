#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include "graphics.h"
#include "timing.h"

void update_cells(uint8_t prev[const], uint8_t next[const]);
inline uint8_t cell_is_alive(const uint8_t cell, const uint8_t num_neighbors);
void draw_cells(uint8_t cells[const], struct sdl_graphics* const gfx);

int main(void)
{
    struct sdl_graphics gfx = init_graphics();

    // Create the cell buffers
    bool draw_from_cellbuf1 = true;
    uint8_t* const cellbuf1 = malloc(NUM_PIXELS);
    uint8_t* const cellbuf2 = malloc(NUM_PIXELS);
    if (cellbuf1 == NULL || cellbuf2 == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for the cell buffers.\n");
        end_graphics(&gfx);
        return EXIT_FAILURE;
    }

    // Randomize cellbuf2
    static const char dev_urand_path[] = "/dev/urandom";
    const int devurand_fd = open(dev_urand_path, O_RDONLY);
    if (devurand_fd == -1) {
        fprintf(stderr, "Error when opening %s: %s\n", dev_urand_path, strerror(errno));
        free(cellbuf1);
        free(cellbuf2);
        end_graphics(&gfx);
        return EXIT_FAILURE;
    }
    if (read(devurand_fd, cellbuf2, NUM_PIXELS) == -1) {
        fprintf(stderr, "Error when reading from %s: %s\n", dev_urand_path, strerror(errno));
    }
    close(devurand_fd);
    for (size_t i = 0; i < NUM_PIXELS; i++) {
        cellbuf2[i] &= 0x01;
    }

    bool quit = false;
    while (!quit) {
        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);

        // UPDATE SCREEN
        if (draw_from_cellbuf1) {
            update_cells(cellbuf2, cellbuf1);
            draw_cells(cellbuf1, &gfx);
        } else {
            update_cells(cellbuf1, cellbuf2);
            draw_cells(cellbuf2, &gfx);
        }
        draw_from_cellbuf1 = !draw_from_cellbuf1;
    
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                case SDLK_SPACE:
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                default:
                    break;
                }
            }
        }

        struct timespec stop;
        clock_gettime(CLOCK_MONOTONIC, &stop);
        const int64_t frame_time_ns = get_time_diff_ns(&start, &stop);
        const int64_t wait_time_ns = NS_PER_FRAME_30FPS - frame_time_ns;
        if (wait_time_ns > 0) {
            struct timespec wait_time = {
                .tv_sec = 0,
                .tv_nsec = wait_time_ns
            };
            nanosleep(&wait_time, NULL);
        }
    }
    
    free(cellbuf1);
    free(cellbuf2);
    end_graphics(&gfx);
    return EXIT_SUCCESS;
}

void update_cells(uint8_t prev[const], uint8_t next[const])
{
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        const bool prev_row_exists = y > 0;
        const bool next_row_exists = y < (SCREEN_HEIGHT - 1);
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const bool prev_col_exists = x > 0;
            const bool next_col_exists = x < (SCREEN_WIDTH - 1);
            const size_t i = row + x;
            uint8_t num_neighbors = (prev_col_exists && prev[i-1]) + (next_col_exists && prev[i+1]);
            if (prev_row_exists) {
                const size_t i_above = i - SCREEN_WIDTH;
                num_neighbors += (prev_col_exists && prev[i_above-1]) + prev[i_above] + (next_col_exists && prev[i_above+1]);
            }
            if (next_row_exists) {
                const size_t i_below = i + SCREEN_WIDTH;
                num_neighbors += (prev_col_exists && prev[i_below-1]) + prev[i_below] + (next_col_exists && prev[i_below+1]);
            }
            next[i] = cell_is_alive(prev[i], num_neighbors);
        }
    }
}

inline uint8_t cell_is_alive(const uint8_t cell, const uint8_t num_neighbors)
{
    return (cell && (num_neighbors == 2 || num_neighbors ==3)) || (!cell && num_neighbors == 3);
}

void draw_cells(uint8_t cells[const], struct sdl_graphics* const gfx)
{
    uint32_t* const pixels = gfx->pixels;
    for (size_t y = 0; y < NUM_PIXELS; y += SCREEN_WIDTH) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const size_t i = y + x;
            pixels[i] = cells[i] ? 0xFF000000 : 0xFFFFFFFF;
        }
    }
    render_graphics(gfx);
}

