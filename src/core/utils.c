/* SPDX-License-Identifier: MIT */
/**
 * @file utils.c
 * @brief Utility functions for argument parsing and string handling.
 */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_utils.h"
#include <string.h>

/** Lightweight argv short-option scanner (stops at "--"). */
int zp_has_option(char **argv, char opt) {
  char *string;
  while ((string = *argv)) {
    if (string[0] == '-') {
      if (string[1] == '-' && string[2] == '\0') {
        return 0;
      }
      while (*++string) {
        if (string[0] == opt) {
          return 1;
        }
      }
    }
    ++argv;
  }
  return 0;
}

/**
 * Consume a short option with required argument from argv.
 * Supports both attached (-oARG) and separate (-o ARG) forms.
 */
int zp_take_opt_with_arg(char ***argvp, char opt, char **out_arg) {
  char **argv = *argvp;
  char *cur = *argv;
  if (!cur || cur[0] != '-' || !cur[1]) {
    return 0; /* not an option */
  }
  if (cur[1] == '-' && cur[2] == '\0') {
    return 0; /* end of options marker */
  }
  if (cur[1] != opt) {
    return 0; /* different option */
  }
  /* attached case: -oARG */
  if (cur[2] != '\0') {
    *out_arg = cur + 2;
    *argvp = argv + 1;
    return 1;
  }
  /* separate case: -o ARG */
  if (!argv[1]) {
    return -1; /* missing argument */
  }
  *out_arg = argv[1];
  *argvp = argv + 2;
  return 1;
}

/** zalloc-backed strdup with length out parameter. */
char *my_ztrdup_glen(const char *s, unsigned *len_ret) {
  char *t;
  if (!s) {
    return NULL;
  }
  *len_ret = strlen((const char *)s);
  t = (char *)zalloc(*len_ret + 1);
  memcpy(t, s, *len_ret);
  t[*len_ret] = '\0';
  return t;
}

/** Duplicate and unmetafy a zsh string with zalloc; see header for details. */
char *zp_unmetafy_zalloc(const char *to_copy, int *new_len) {
  char *work;
  char *to_return;
  int my_new_len = 0;
  unsigned meta_length = 0;
  work = my_ztrdup_glen(to_copy, &meta_length);
  if (!work) {
    return NULL;
  }
  work = unmetafy(work, &my_new_len);
  if (new_len) {
    *new_len = my_new_len;
  }
  to_return = (char *)zalloc((my_new_len + 1) * sizeof(char));
  if (!to_return) {
    zfree(work, meta_length);
    return NULL;
  }
  memcpy(to_return, work, sizeof(char) * my_new_len);
  to_return[my_new_len] = '\0';
  zfree(work, meta_length);
  return to_return;
}
