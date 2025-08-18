/* SPDX-License-Identifier: MIT */
#pragma once

/* Utility helpers shared across module components */

/* Duplicate and unmetafy a zsh string using zsh allocators; returns new
 * buffer that must be zfree'd with the exact length returned via new_len. */
char *zp_unmetafy_zalloc(const char *to_copy, int *new_len);

/* Duplicate using zalloc and return length via len_ret; must be zfree'd
 * with the length provided. */
char *my_ztrdup_glen(const char *s, unsigned *len_ret);

/* Lightweight option scanner for argv-style subcommands. */
int zp_has_option(char **argv, char opt);
