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

#define LIVE_CELL 0xFF000000
#define DEAD_CELL 0xFFFFFFFF

void update_cells(uint32_t prev[const static NUM_PIXELS], uint32_t next[const static NUM_PIXELS]);
void update_cells_alt(uint32_t cells[const static NUM_PIXELS], uint32_t neighbor_counts[const static NUM_PIXELS]);
inline bool cell_is_alive(const uint32_t cell, const uint8_t num_neighbors);

#ifdef __ARM_NEON__
#include <arm_neon.h>
void update_cells_neon(uint32_t cells[const static NUM_PIXELS], uint32_t neighbor_counts[const static NUM_PIXELS]);
#endif

int main(void)
{
    struct sdl_graphics gfx = init_graphics("Conway's Game of Life");

    // Create the cell buffers
    //uint32_t* const cellbuf1 = malloc(NUM_BYTES_IN_TEXTURE);
    uint32_t* const cellbuf2 = malloc(NUM_BYTES_IN_TEXTURE);
    uint32_t* const neighbor_counts = malloc(NUM_BYTES_IN_TEXTURE);
    if (/*(cellbuf1 == NULL) ||*/ (cellbuf2 == NULL) || (neighbor_counts == NULL)) {
        fprintf(stderr, "Error: Failed to allocate memory for the cell buffers.\n");
        end_graphics(&gfx);
        return EXIT_FAILURE;
    }

    // Randomize cellbuf2
    static const char dev_urand_path[] = "/dev/urandom";
    const int devurand_fd = open(dev_urand_path, O_RDONLY);
    if (devurand_fd == -1) {
        fprintf(stderr, "Error when opening %s: %s\n", dev_urand_path, strerror(errno));
        //free(cellbuf1);
        free(cellbuf2);
        free(neighbor_counts);
        end_graphics(&gfx);
        return EXIT_FAILURE;
    }
    if (read(devurand_fd, cellbuf2, NUM_BYTES_IN_TEXTURE) == -1) {
        fprintf(stderr, "Error when reading from %s: %s\n", dev_urand_path, strerror(errno));
    }
    close(devurand_fd);
    size_t i = 0;
#ifdef __ARM_NEON__
    const uint32x4_t all_live = vdupq_n_u32(LIVE_CELL);
    const uint32x4_t all_dead = vdupq_n_u32(DEAD_CELL);
    const uint32x4_t one_vec = vdupq_n_u32(0x00000001);
    for (; (i + 4) <= NUM_PIXELS; i += 4) {
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
        /*
        static bool draw_from_cellbuf1 = true;
        if (draw_from_cellbuf1) {
            update_cells(cellbuf2, cellbuf1);
            render_graphics(&gfx, cellbuf1);
        } else {
            update_cells(cellbuf1, cellbuf2);
            render_graphics(&gfx, cellbuf2);
        }
        draw_from_cellbuf1 = !draw_from_cellbuf1;
        */
#ifdef __ARM_NEON__
        update_cells_neon(cellbuf2, neighbor_counts);
#else
        update_cells_alt(cellbuf2, neighbor_counts);
#endif
        render_graphics(&gfx, cellbuf2);

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
        //printf("Frame time: %" PRIi64 " ns\n", frame_time_ns);
        const int64_t wait_time_ns = NS_PER_FRAME_30FPS - frame_time_ns;
        if (wait_time_ns > 0) {
            struct timespec wait_time = {
                .tv_sec = 0,
                .tv_nsec = wait_time_ns
            };
            nanosleep(&wait_time, NULL);
        }
    }
    
    //free(cellbuf1);
    free(cellbuf2);
    free(neighbor_counts);
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
    return (num_neighbors == 3) || ((cell == LIVE_CELL) && (num_neighbors == 2));
}

void update_cells_alt(
    uint32_t cells[const static NUM_PIXELS],
    uint32_t neighbor_counts[const static NUM_PIXELS])
{
    // Up left
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 0; x < SCREEN_WIDTH - 1; x++) {
            const size_t i = row + x;
            neighbor_counts[i + SCREEN_WIDTH + 1] = (cells[i] == LIVE_CELL);
        }
    }

    // Up
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i + SCREEN_WIDTH] += (cells[i] == LIVE_CELL);
        }
    }

    // Up right
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 1; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i + SCREEN_WIDTH - 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Left
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 0; x < SCREEN_WIDTH - 1; x++) {
            const size_t i = row + x;
            neighbor_counts[i + 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Right
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 1; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i - 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Down left
    for (size_t y = 1; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 0; x < SCREEN_WIDTH - 1; x++) {
            const size_t i = row + x;
            neighbor_counts[i - SCREEN_WIDTH + 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Down
    for (size_t y = 1; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i - SCREEN_WIDTH] += (cells[i] == LIVE_CELL);
        }
    }

    // Down right
    for (size_t y = 1; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 1; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i - SCREEN_WIDTH - 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Update cells
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            const uint32_t num_neighbors = neighbor_counts[i];
            if ((num_neighbors == 3) || ((cells[i] == LIVE_CELL) && (num_neighbors == 2))) {
                cells[i] = LIVE_CELL;
            } else {
                cells[i] = DEAD_CELL;
            }
        }
    }
}

#if defined(__ARM_NEON__)
void update_cells_neon(
    uint32_t cells[const static NUM_PIXELS],
    uint32_t neighbor_counts[const static NUM_PIXELS])
{
    const uint32x4_t live_vec = vdupq_n_u32(LIVE_CELL);
    const uint32x4_t one_vec = vdupq_n_u32(1);

    // Up left
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 0;
        for (; (x + 4) <= SCREEN_WIDTH - 1; x += 4) {
            const size_t i = row + x;
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            vst1q_u32(neighbor_counts + i + SCREEN_WIDTH + 1, vandq_u32(is_alive_vec, one_vec));
        }
        for (; x < SCREEN_WIDTH - 1; x++) {
            const size_t i = row + x;
            neighbor_counts[i + SCREEN_WIDTH + 1] = (cells[i] == LIVE_CELL);
        }
    }

    // Up
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 0;
        for (; (x + 4) <= SCREEN_WIDTH; x += 4) {
            const size_t i = row + x;
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            const uint32x4_t inc_vec = vandq_u32(is_alive_vec, one_vec);
            const size_t i_down = i + SCREEN_WIDTH;
            const uint32x4_t num_neighbors_vec = vld1q_u32(neighbor_counts + i_down);
            vst1q_u32(neighbor_counts + i_down, vaddq_u32(num_neighbors_vec, inc_vec));
        }
        for (; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i + SCREEN_WIDTH] += (cells[i] == LIVE_CELL);
        }
    }

    // Up right
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 1;
        for (; (x + 4) <= SCREEN_WIDTH; x += 4) {
            const size_t i = row + x;
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            const uint32x4_t inc_vec = vandq_u32(is_alive_vec, one_vec);
            const size_t i_down_left = i + SCREEN_WIDTH - 1;
            const uint32x4_t num_neighbors_vec = vld1q_u32(neighbor_counts + i_down_left);
            vst1q_u32(neighbor_counts + i_down_left, vaddq_u32(num_neighbors_vec, inc_vec));
        }
        for (; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i + SCREEN_WIDTH - 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Left
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 0;
        for (; (x + 4) <= SCREEN_WIDTH - 1; x += 4) {
            const size_t i = row + x;
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            const uint32x4_t inc_vec = vandq_u32(is_alive_vec, one_vec);
            const size_t i_right = i + 1;
            const uint32x4_t num_neighbors_vec = vld1q_u32(neighbor_counts + i_right);
            vst1q_u32(neighbor_counts + i_right, vaddq_u32(num_neighbors_vec, inc_vec));
        }
        for (; x < SCREEN_WIDTH - 1; x++) {
            const size_t i = row + x;
            neighbor_counts[i + 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Right
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 1;
        for (; (x + 4) <= SCREEN_WIDTH; x += 4) {
            const size_t i = row + x;
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            const uint32x4_t inc_vec = vandq_u32(is_alive_vec, one_vec);
            const size_t i_left = i - 1;
            const uint32x4_t num_neighbors_vec = vld1q_u32(neighbor_counts + i_left);
            vst1q_u32(neighbor_counts + i_left, vaddq_u32(num_neighbors_vec, inc_vec));
        }
        for (; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i - 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Down left
    for (size_t y = 1; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 0;
        for (; (x + 4) <= SCREEN_WIDTH - 1; x += 4) {
            const size_t i = row + x;
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            const uint32x4_t inc_vec = vandq_u32(is_alive_vec, one_vec);
            const size_t i_up_right = i - SCREEN_WIDTH + 1;
            const uint32x4_t num_neighbors_vec = vld1q_u32(neighbor_counts + i_up_right);
            vst1q_u32(neighbor_counts + i_up_right, vaddq_u32(num_neighbors_vec, inc_vec));
        }
        for (; x < SCREEN_WIDTH - 1; x++) {
            const size_t i = row + x;
            neighbor_counts[i - SCREEN_WIDTH + 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Down
    for (size_t y = 1; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 0;
        for (; (x + 4) <= SCREEN_WIDTH; x += 4) {
            const size_t i = row + x;
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            const uint32x4_t inc_vec = vandq_u32(is_alive_vec, one_vec);
            const size_t i_up = i - SCREEN_WIDTH;
            const uint32x4_t num_neighbors_vec = vld1q_u32(neighbor_counts + i_up);
            vst1q_u32(neighbor_counts + i_up, vaddq_u32(num_neighbors_vec, inc_vec));
        }
        for (; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i - SCREEN_WIDTH] += (cells[i] == LIVE_CELL);
        }
    }

    // Down right
    for (size_t y = 1; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 1;
        for (; (x + 4) <= SCREEN_WIDTH; x += 4) {
            const size_t i = row + x;
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            const uint32x4_t inc_vec = vandq_u32(is_alive_vec, one_vec);
            const size_t i_up_left = i - SCREEN_WIDTH - 1;
            const uint32x4_t num_neighbors_vec = vld1q_u32(neighbor_counts + i_up_left);
            vst1q_u32(neighbor_counts + i_up_left, vaddq_u32(num_neighbors_vec, inc_vec));
        }
        for (; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            neighbor_counts[i - SCREEN_WIDTH - 1] += (cells[i] == LIVE_CELL);
        }
    }

    // Update cells
    const uint32x4_t three_vec = vdupq_n_u32(3);
    const uint32x4_t two_vec = vdupq_n_u32(2);
    const uint32x4_t dead_vec = vdupq_n_u32(DEAD_CELL);
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const size_t row = y * SCREEN_WIDTH;
        size_t x = 0;
        for (; (x + 4) <= SCREEN_WIDTH; x += 4) {
            const size_t i = row + x;
            const uint32x4_t num_neighbors_vec = vld1q_u32(neighbor_counts + i);
            const uint32x4_t has_three_neighbors_vec = vceqq_u32(num_neighbors_vec, three_vec);
            const uint32x4_t has_two_neighbors_vec = vceqq_u32(num_neighbors_vec, two_vec);
            const uint32x4_t cell_vec = vld1q_u32(cells + i);
            const uint32x4_t is_alive_vec = vceqq_u32(cell_vec, live_vec);
            const uint32x4_t should_live_vec = vorrq_u32(has_three_neighbors_vec, vandq_u32(is_alive_vec, has_two_neighbors_vec));
            const uint32x4_t should_die_vec = vmvnq_u32(should_live_vec);
            const uint32x4_t new_cell_vec = vorrq_u32(
                vandq_u32(should_die_vec, dead_vec),
                vandq_u32(should_live_vec, live_vec)
            );
            vst1q_u32(cells + i, new_cell_vec);
        }
        for (; x < SCREEN_WIDTH; x++) {
            const size_t i = row + x;
            const uint32_t num_neighbors = neighbor_counts[i];
            if ((num_neighbors == 3) || ((cells[i] == LIVE_CELL) && (num_neighbors == 2))) {
                cells[i] = LIVE_CELL;
            } else {
                cells[i] = DEAD_CELL;
            }
        }
    }
}
#endif

