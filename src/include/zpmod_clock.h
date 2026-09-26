/* SPDX-License-Identifier: MIT */
#pragma once
#include <limits.h>
#include <time.h>

/* Convert an elapsed interval without truncating sub-millisecond durations. */
static inline int zp_elapsed_ns(time_t start_sec, long start_ns, time_t end_sec, long end_ns, unsigned long long *elapsed)
{
    if (start_sec < 0 || end_sec < start_sec || start_ns < 0 || start_ns >= 1000000000L || end_ns < 0 || end_ns >= 1000000000L ||
        (end_sec == start_sec && end_ns < start_ns)) {
        return 0;
    }
    unsigned long long seconds = (unsigned long long)end_sec - (unsigned long long)start_sec;
    long nanos = end_ns - start_ns;
    if (nanos < 0) {
        --seconds;
        nanos += 1000000000L;
    }
    if (seconds > (ULLONG_MAX - (unsigned long long)nanos) / 1000000000ULL) {
        return 0;
    }
    *elapsed = seconds * 1000000000ULL + (unsigned long long)nanos;
    return 1;
}
