#include "timing.h"

struct timespec get_time_diff(struct timespec const* const start, struct timespec const* const stop)
{
    struct timespec diff = {
        .tv_sec = stop->tv_sec - start->tv_sec,
        .tv_nsec = stop->tv_nsec - start->tv_nsec
    };
    if (diff.tv_sec > 0 && diff.tv_nsec < 0) {
        diff.tv_nsec += NS_PER_S;
        diff.tv_sec--;
    } else if (diff.tv_sec < 0 && diff.tv_nsec > 0) {
        diff.tv_nsec -= NS_PER_S;
        diff.tv_sec++;
    }
    return diff;
}

int64_t get_time_diff_ns(struct timespec const* const start, struct timespec const* const stop)
{
    struct timespec const diff = get_time_diff(start, stop);
    return (diff.tv_sec * NS_PER_S) + diff.tv_nsec;
}

