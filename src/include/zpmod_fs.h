/* SPDX-License-Identifier: MIT */
#pragma once

/**
 * @file zpmod_fs.h
 * @brief Filesystem helpers used by builtins and `zpmod` subcommands.
 *
 * These functions perform the core work for filesystem-related builtins.
 * They follow zsh module conventions:
 * - All string allocations use zsh allocators (zalloc/zsfree/zfree).
 * - When a function returns a newly allocated buffer and also provides the
 *   buffer length, call sites must free with the exact length.
 * - Parameters named `nam` are the reporting name (builtin/subcommand) used
 *   with zsh’s zwarn/zerr helpers.
 *
 * @ingroup core
 */

/**
 * @brief Batch stat entries from an input array into an output array.
 *
 * For each path in `inname`, writes a metafied key=value record into
 * `outname`. The set of fields can be filtered via `fields`.
 *
 * @param nam     Reporting name for diagnostics (e.g., builtin name).
 * @param outname Name of the output indexed array parameter to populate.
 * @param inname  Name of the input indexed array parameter to read.
 * @param follow  When non-zero, follow symlinks (stat); otherwise lstat.
 * @param fields  Optional comma-separated field filter (e.g., "type,size").
 * @return 0 on success; non-zero on error (zwarn already emitted).
 */
/* Parameter order is (reporting name, output array name, input array name,
 * follow-symlinks, fields) */
int zp_pathstat_core(char *nam, char *outname, char *inname, int follow,
                     char *fields);

/**
 * @brief List directory entries into an output array with filters.
 *
 * @param nam        Reporting name for diagnostics.
 * @param outname    Name of the output indexed array parameter to populate.
 * @param dir        Directory to scan (metafied string).
 * @param inc_all    Include dotfiles when non-zero.
 * @param only_dirs  Include only directories when non-zero.
 * @param only_files Include only regular files when non-zero.
 * @return 0 on success; non-zero on error.
 */
/* Parameter order is (reporting name, output array name, directory,
 * include-dotfiles, only-dirs, only-files) */
int zp_dirlist_core(char *nam, char *outname, char *dir, int inc_all,
                    int only_dirs, int only_files);

/**
 * @brief Read a file into a scalar or split into an array by delimiter.
 *
 * @param nam      Reporting name for diagnostics.
 * @param outname  Output parameter name: scalar (no split) or array (split).
 * @param path     Path of file to read (metafied string).
 * @param use_mmap Hint to use mmap when available and beneficial.
 * @param split    When non-zero, split on delimiter into an array.
 * @param delim    Delimiter character when splitting (e.g., '\n').
 * @return 0 on success; non-zero on error.
 */
/* Parameter order is (reporting name, output var/array, path, use-mmap,
 * split-mode, delimiter) */
int zp_readfile_core(char *nam, char *outname, char *path, int use_mmap,
                     int split, int delim);
