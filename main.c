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

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#define LIVE_CELL 0xFF000000
#define DEAD_CELL 0xFFFFFFFF

void update_cells(uint32_t prev[const static NUM_PIXELS], uint32_t next[const static NUM_PIXELS]);
inline bool cell_is_alive(const uint32_t cell, const uint8_t num_neighbors);

int main(void)
{
    struct sdl_graphics gfx = init_graphics("Conway's Game of Life");

    // Create the cell buffers
    bool draw_from_cellbuf1 = true;
    uint32_t* const cellbuf1 = malloc(NUM_BYTES);
    uint32_t* const cellbuf2 = malloc(NUM_BYTES);
    if ((cellbuf1 == NULL) || (cellbuf2 == NULL)) {
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
    if (read(devurand_fd, cellbuf2, NUM_BYTES) == -1) {
        fprintf(stderr, "Error when reading from %s: %s\n", dev_urand_path, strerror(errno));
    }
    close(devurand_fd);
    size_t i = 0;
#ifdef __ARM_NEON__
    const uint32x4_t all_live = vdupq_n_u32(LIVE_CELL);
    const uint32x4_t all_dead = vdupq_n_u32(DEAD_CELL);
    const uint32x4_t one_vec = vdupq_n_u32(0x00000001);
    for (; (i + 4) < NUM_PIXELS; i += 4) {
        uint32_t* const ptr_cells = cellbuf2 + i;
        const uint32x4_t current_cells = vandq_u32(vld1q_u32(ptr_cells), one_vec);
        const uint32x4_t live_mask = vceqq_u32(current_cells, one_vec);
        const uint32x4_t dead_mask = vmvnq_u32(live_mask);
        const uint32x4_t live_cells = vandq_u32(live_mask, all_live);
        const uint32x4_t dead_cells = vandq_u32(dead_mask, all_dead);
        vst1q_u32(ptr_cells, vorrq_u32(live_cells, dead_cells));
    }
#else
    for (; i < NUM_PIXELS; i++) {
        if (cellbuf2[i] & 0x00000001) {
            cellbuf2[i] = LIVE_CELL;
        } else {
            cellbuf2[i] = DEAD_CELL;
        }
    }
#endif

    bool quit = false;
    while (!quit) {
        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);

        // UPDATE SCREEN
        if (draw_from_cellbuf1) {
            update_cells(cellbuf2, cellbuf1);
            render_graphics(&gfx, cellbuf1);
        } else {
            update_cells(cellbuf1, cellbuf2);
            render_graphics(&gfx, cellbuf2);
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

void update_cells(uint32_t prev[const static NUM_PIXELS], uint32_t next[const static NUM_PIXELS])
{
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        const bool prev_row_exists = y > 0;
        const bool next_row_exists = y < (SCREEN_HEIGHT - 1);
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const bool prev_col_exists = x > 0;
            const bool next_col_exists = x < (SCREEN_WIDTH - 1);
            const size_t i = row + x;
            uint8_t num_neighbors = (prev_col_exists && (prev[i-1] == LIVE_CELL)) + (next_col_exists && (prev[i+1] == LIVE_CELL));
            if (prev_row_exists) {
                const size_t i_above = i - SCREEN_WIDTH;
                num_neighbors += (prev_col_exists && (prev[i_above-1] == LIVE_CELL))
                               + (prev[i_above] == LIVE_CELL)
                               + (next_col_exists && (prev[i_above+1] == LIVE_CELL));
            }
            if (next_row_exists) {
                const size_t i_below = i + SCREEN_WIDTH;
                num_neighbors += (prev_col_exists && (prev[i_below-1] == LIVE_CELL))
                               + (prev[i_below] == LIVE_CELL)
                               + (next_col_exists && (prev[i_below+1] == LIVE_CELL));
            }
            if (cell_is_alive(prev[i], num_neighbors)) {
                next[i] = LIVE_CELL;
            } else {
                next[i] = DEAD_CELL;
            }
        }
    }
}

inline bool cell_is_alive(const uint32_t cell, const uint8_t num_neighbors)
{
    //return (cell && ((num_neighbors == 2) || (num_neighbors == 3))) || (!cell && (num_neighbors == 3));
    return ((cell == LIVE_CELL) && (num_neighbors == 2)) || (num_neighbors == 3);
}

