/* SPDX-License-Identifier: MIT */
#pragma once

/**
 * @file zpmod_utils.h
 * @brief Utility helpers shared across module components.
 *
 * These helpers follow zpmod’s memory rules: allocate with zsh allocators,
 * and free with exact lengths where applicable.
 */

/**
 * @brief Duplicate and unmetafy a zsh string using zsh allocators.
 *
 * Makes a new allocation with zalloc, containing the unmetafied copy of
 * `to_copy`. When `new_len` is provided, it receives the exact buffer length
 * to use with zfree.
 *
 * @param to_copy Metafied zsh string to duplicate and unmetafy.
 * @param new_len Optional out parameter for the returned buffer length.
 * @return Newly allocated buffer (must be freed with zfree using length), or
 *         NULL on allocation failure.
 */
char *zp_unmetafy_zalloc(const char *to_copy, int *new_len);

/**
 * @brief Duplicate a C string using zalloc and report length.
 *
 * @param s       Source string (may be NULL).
 * @param len_ret Out parameter receiving the allocated buffer length.
 * @return Newly allocated duplicate (must be freed with zfree using
 *         `*len_ret`), or NULL if `s` is NULL.
 */
char *my_ztrdup_glen(const char *s, unsigned *len_ret);

/**
 * @brief Lightweight option scanner for argv-style subcommands.
 *
 * Scans `argv` for a short option (e.g., 'l' for -l).
 * Stops at "--" if encountered.
 *
 * @param argv Argument vector terminated by NULL.
 * @param opt  Short option character to detect.
 * @return 1 if present, 0 otherwise.
 */
int zp_has_option(char **argv, char opt);
