#include "timer.h"

u_int64_t get_monotonic_msec(){
    struct timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return u_int64_t(tv.tv_sec) * 1000 + tv.tv_nsec / 1000 / 1000;
};