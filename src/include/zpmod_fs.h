/* SPDX-License-Identifier: MIT */
#pragma once

/* Filesystem helpers used by builtins and subcommands */
int zp_pathstat_core(char *nam, char *outname, char *inname, int follow,
                     char *fields);
int zp_dirlist_core(char *nam, char *outname, char *dir, int inc_all,
                    int only_dirs, int only_files);
int zp_readfile_core(char *nam, char *outname, char *path, int use_mmap,
                     int split, int delim);
