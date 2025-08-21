/* SPDX-License-Identifier: MIT */
/**
 * @file fs_builtins.c
 * @brief Builtins: zppathstat, zpdirlist, zpreadfile.
 */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_fs.h"
#include "zpmod_emoji.h"

/** zppathstat builtin entrypoint */
int bin_zppathstat(char *nam, char **argv, UNUSED(Options ops),
                   UNUSED(int func)) {
  int follow = OPT_ISSET(ops, 'L');
  char *fields = NULL;
  if (OPT_ISSET(ops, 'f')) {
    fields = OPT_ARG(ops, 'f');
  }
  if (!argv || !argv[0] || !argv[1]) {
    zwarnnam(nam, "%s %susage: %s [-L] [-f fields] out_array in_array", zp_icon("⚠️ "), nam, nam);
    return 1;
  }
  return zp_pathstat_core(nam, argv[0], argv[1], follow, fields);
}

/** zpdirlist builtin entrypoint */
int bin_zpdirlist(char *nam, char **argv, UNUSED(Options ops),
                  UNUSED(int func)) {
  int inc_all = OPT_ISSET(ops, 'a');
  int only_dirs = OPT_ISSET(ops, 'd');
  int only_files = OPT_ISSET(ops, 'f');
  if (!argv || !argv[0] || !argv[1]) {
    zwarnnam(nam, "%s %susage: %s [-a] [-d] [-f] out_array dir", zp_icon("⚠️ "), nam, nam);
    return 1;
  }
  return zp_dirlist_core(nam, argv[0], argv[1], inc_all, only_dirs, only_files);
}

/** zpreadfile builtin entrypoint */
int bin_zpreadfile(char *nam, char **argv, UNUSED(Options ops),
                   UNUSED(int func)) {
  int use_mmap = OPT_ISSET(ops, 'm');
  int delim = '\n';
  int split = 0;
  if (OPT_ISSET(ops, '0')) {
    split = 1;
    delim = '\0';
  }
  if (OPT_ISSET(ops, 'd')) {
    char *a = OPT_ARG(ops, 'd');
    if (a && *a) {
      split = 1;
      if (a[0] == '\\') {
        switch (a[1]) {
        case 'n':
          delim = '\n';
          break;
        case 't':
          delim = '\t';
          break;
        case '0':
          delim = '\0';
          break;
        case 'r':
          delim = '\r';
          break;
        default:
          delim = (unsigned char)a[1];
          break;
        }
      } else {
        delim = (unsigned char)a[0];
      }
    } else {
      split = 1;
      delim = '\n';
    }
  }
  if (!argv || !argv[0] || !argv[1]) {
    zwarnnam(nam, "%s %susage: %s [-m] [-d delim|-0] out file", zp_icon("⚠️ "), nam, nam);
    return 1;
  }
  return zp_readfile_core(nam, argv[0], argv[1], use_mmap, split, delim);
}

/* Export table from this TU */
static struct builtin fs_builtins[] = {
    BUILTIN("zppathstat", 0, bin_zppathstat, 2, 2, 0, "Lf:", NULL),
    BUILTIN("zpdirlist", 0, bin_zpdirlist, 2, 2, 0, "adf", NULL),
    BUILTIN("zpreadfile", 0, bin_zpreadfile, 2, 2, 0, "md:0", NULL),
};

struct builtin *zp_get_fs_builtins(size_t *count) {
  if (count) {
    *count = sizeof(fs_builtins) / sizeof(*fs_builtins);
  }
  return fs_builtins;
}
