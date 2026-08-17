/* SPDX-License-Identifier: MIT */
#pragma once
#include <stddef.h>

/* Accessors to builtin slices from separate TUs */
struct builtin;
struct builtin *zp_get_fs_builtins(size_t *count);

/* Module entrypoints (hooks) */
int setup_(Module m);
int features_(Module m, char ***features);
int enables_(Module m, int **enables);
int boot_(Module m);
int cleanup_(Module m);
int finish_(Module m);
