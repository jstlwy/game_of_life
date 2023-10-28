#ifndef timing_h
#define timing_h

#include <stdint.h>
#include <time.h>

#define NS_PER_S           1000000000
#define NS_PER_FRAME_15FPS 66666667
#define NS_PER_FRAME_30FPS 33333333
#define NS_PER_FRAME_60FPS 16666667

struct timespec get_time_diff_struct(const struct timespec* const start, const struct timespec* const stop);
int64_t get_time_diff_ns(const struct timespec* const start, const struct timespec* const stop);

#endif

