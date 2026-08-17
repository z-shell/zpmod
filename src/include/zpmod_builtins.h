/* SPDX-License-Identifier: MIT */
#pragma once
#include <stddef.h>
struct builtin;

/* Accessors for builtin slices */
struct builtin *zp_get_self_builtins(size_t *count);
struct builtin *zp_get_fs_builtins(size_t *count);

/* Other builtins implemented as direct handlers (zpreadarray, custom_dot) */
int bin_zpreadarray(char *nam, char **argv, Options ops, int func);
int bin_custom_dot(char *name, char **argv, Options ops, int func);
int bin_zpmod(char *nam, char **argv, Options ops, int func);
