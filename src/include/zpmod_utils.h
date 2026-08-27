/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdio.h>
#include <sys/types.h>

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

/**
 * @brief Consume a short option that requires an argument from argv.
 *
 * Recognizes patterns "-oARG" (attached) and "-o ARG" (separate).
 * When matched, advances the argv pointer and writes the argument to out_arg.
 *
 * @param argvp   In/out pointer to argv cursor (modified on consumption).
 * @param opt     Short option character to match (e.g., 'd').
 * @param out_arg Out parameter set to the option argument on success.
 * @return 1 if option was present and consumed successfully,
 *         0 if the current argv element does not match the option,
 *        -1 if the option matched but the argument was missing.
 */
int zp_take_opt_with_arg(char ***argvp, char opt, char **out_arg);

/**
 * @brief Open a file for truncating writes without following symlinks.
 *
 * The file receives `create_mode` at creation time, independent of a permissive
 * process umask. Existing file permissions are preserved unless
 * `enforce_mode` is non-zero, in which case the opened descriptor is restricted
 * to `create_mode` before a stream is returned.
 *
 * @param path         Output path.
 * @param create_mode  Permissions used when creating the file.
 * @param enforce_mode Whether to apply create_mode to an existing file too.
 * @return Writable stream, or NULL with errno preserved on failure.
 */
FILE *zp_fopen_write_nofollow(const char *path, mode_t create_mode,
                              int enforce_mode);
