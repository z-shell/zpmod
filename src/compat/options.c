/* SPDX-License-Identifier: MIT */
/**
 * @file options.c
 * @brief Stable-to-runtime option mapping across zsh versions.
 *
 * Provides `zp_setup_options_table()` to resolve version-stable enum values
 * to runtime option indices using zsh’s `optlookup()`, and `zp_conv_opt()` to
 * convert an enum for use with `isset()`/`dosetopt()`.
 */
#include "zpmod.mdh"
#include "zpmod.pro"
#include <stddef.h>
#include "zpmod_compat.h"

static int zp_opt_for_zsh_version[64] = {0};

struct zp_option_name {
  const char *name;
  int enum_val;
};

/*
 * Mapping from zpmod-stable option enums to zsh option names.
 * Keep this list terminated with a NULL name sentinel.
 */
static struct zp_option_name zp_options[] = {
  /* Only map the options we actually use from C code. */
  {"function_argzero", FUNCTIONARGZERO__},
  {"path_dirs", PATHDIRS__},
  {"posix_builtins", POSIXBUILTINS__},
  {"shin_stdin", SHINSTDIN__},
  {"source_trace", SOURCETRACE__},
  /* Sentinel terminator (required) */
  {NULL, 0}};

/** Populate runtime option indices for the stable enum table. */
void zp_setup_options_table(void) {
  for (int i = 0; zp_options[i].name != NULL; ++i) {
    int e = zp_options[i].enum_val;
    if (e < 0 || e >= (int)(sizeof(zp_opt_for_zsh_version) /
                            sizeof(zp_opt_for_zsh_version[0]))) {
      continue;
    }
    int optno = optlookup(zp_options[i].name);
    if (optno >= 0) {
      zp_opt_for_zsh_version[e] = optno;
    }
  }
}

/** Convert a stable option enum to a runtime option index (sign-preserving). */
int zp_conv_opt(int zp_opt_num) {
  if (zp_opt_num < 0 || zp_opt_num >= ZP_OPT_COUNT__) {
    return 0;
  }
  return zp_opt_for_zsh_version[zp_opt_num];
}
