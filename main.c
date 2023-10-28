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
    struct sdl_graphics gfx;
    init_graphics(&gfx);

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
    static const size_t LAST_COL = SCREEN_WIDTH - 1;
    uint8_t num_neighbors;

    // TOP ROW
    // Top left corner cell
    num_neighbors = prev[1] + prev[SCREEN_WIDTH] + prev[SCREEN_WIDTH+1];
    next[0] = cell_is_alive(prev[0], num_neighbors);
    // Second through second-to-last cells
    for (size_t x = 1; x < LAST_COL; x++) {
        const size_t i_below = x + SCREEN_WIDTH;
        num_neighbors = prev[x-1] + prev[x+1] + prev[i_below-1] + prev[i_below] + prev[i_below+1];
        next[x] = cell_is_alive(prev[x], num_neighbors);
    }
    // Top right corner cell
    static const size_t i_below_trc = LAST_COL + SCREEN_WIDTH;
    num_neighbors = prev[LAST_COL-1] + prev[i_below_trc] + prev[i_below_trc-1];
    next[LAST_COL] = cell_is_alive(prev[LAST_COL], num_neighbors);

    // SECOND THROUGH SECOND-TO-LAST ROWS
    for (size_t y = SCREEN_WIDTH; y < NUM_PIXELS - SCREEN_WIDTH; y += SCREEN_WIDTH) {
        // FAR LEFT PIXEL
        size_t i_above = y - SCREEN_WIDTH;
        size_t i_below = y + SCREEN_WIDTH;
        num_neighbors = prev[i_above] + prev[i_above+1] + prev[y+1] + prev[i_below] + prev[i_below+1];
        next[y] = cell_is_alive(prev[y], num_neighbors);

        // MIDDLE PIXELS
        for (size_t x = 1; x < SCREEN_WIDTH - 1; x++) {
            const size_t i_cell = y + x;
            i_above = i_cell - SCREEN_WIDTH;
            i_below = i_cell + SCREEN_WIDTH;
            num_neighbors = prev[i_cell-1] + prev[i_cell+1] +
                            prev[i_above-1] + prev[i_above] + prev[i_above+1] +
                            prev[i_below-1] + prev[i_below] + prev[i_below+1];
            next[i_cell] = cell_is_alive(prev[i_cell], num_neighbors);
        }

        // FAR RIGHT PIXEL
        const size_t i_cell = y + LAST_COL;
        i_above = i_cell - SCREEN_WIDTH;
        i_below = i_cell + SCREEN_WIDTH;
        num_neighbors = prev[i_above] + prev[i_above-1] + prev[i_cell-1] + prev[i_below] + prev[i_below-1];
        next[y] = cell_is_alive(prev[i_cell], num_neighbors);
    }

    // BOTTOM ROW
    // Bottom left corner cell
    static const size_t y_bottom = NUM_PIXELS - SCREEN_WIDTH;
    num_neighbors = prev[y_bottom+1] + prev[y_bottom-SCREEN_WIDTH] + prev[y_bottom-SCREEN_WIDTH+1];
    next[y_bottom] = cell_is_alive(prev[y_bottom], num_neighbors);
    // Second through second-to-last cells
    for (size_t x = 1; x < SCREEN_WIDTH; x++) {
        const size_t i_cell = y_bottom + x;
        const size_t i_above = i_cell - SCREEN_WIDTH;
        num_neighbors = prev[i_cell-1] + prev[i_cell+1] + prev[i_above-1] + prev[i_above] + prev[i_above+1];
        next[i_cell] = cell_is_alive(prev[i_cell], num_neighbors);
    }
    // Bottom right corner cell
    static const size_t i_brc = NUM_PIXELS-1;
    static const size_t i_above_brc = i_brc - SCREEN_WIDTH;
    num_neighbors = prev[i_brc-1] + prev[i_above_brc] + prev[i_above_brc-1];
    next[i_brc] = cell_is_alive(prev[i_brc], num_neighbors);
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

