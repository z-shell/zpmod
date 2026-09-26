/* SPDX-License-Identifier: MIT */
#include "zpmod_clock.h"

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)

int main(void)
{
    unsigned long long elapsed = 0;
    CHECK(zp_elapsed_ns(10, 100, 10, 223, &elapsed) && elapsed == 123);
    CHECK(zp_elapsed_ns(10, 999999999L, 11, 0, &elapsed) && elapsed == 1);
    CHECK(zp_elapsed_ns(10, 0, 11, 0, &elapsed) && elapsed == 1000000000ULL);
    CHECK(zp_elapsed_ns(10, 0, 10, 0, &elapsed) && elapsed == 0);
    CHECK(!zp_elapsed_ns(10, 1, 10, 0, &elapsed));
    CHECK(!zp_elapsed_ns(10, 0, 9, 999999999L, &elapsed));
    CHECK(!zp_elapsed_ns(10, -1, 11, 0, &elapsed));
    CHECK(!zp_elapsed_ns(10, 0, 11, 1000000000L, &elapsed));
    return 0;
}
