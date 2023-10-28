#include "timing.h"

struct timespec get_time_diff_struct(const struct timespec* const start, const struct timespec* const stop)
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

int64_t get_time_diff_ns(const struct timespec* const start, const struct timespec* const stop)
{
    int64_t sec = stop->tv_sec - start->tv_sec;
    int64_t nsec = stop->tv_nsec - start->tv_nsec;
    if (sec > 0 && nsec < 0) {
        nsec += NS_PER_S;
        sec--;
    } else if (sec < 0 && nsec > 0) {
        nsec -= NS_PER_S;
        sec++;
    }
    return (sec * NS_PER_S) + nsec;
}

