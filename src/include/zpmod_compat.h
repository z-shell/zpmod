/* SPDX-License-Identifier: MIT */
#pragma once

/* Stable-to-runtime option mapping for zsh variants */
void zp_setup_options_table(void);
int zp_conv_opt(int zp_opt_num);

/*
 * Minimal version-stable option identifiers (all suffixed with __) that
 * zpmod uses. Keep this list minimal to avoid upstream enum collisions
 * with vendor/zsh headers. Only add a new enumerator when it is referenced
 * in C code or tests. Never mirror the full upstream option list here.
 */
enum {
  ZP_OPT_INVALID__ = 0,
  FUNCTIONARGZERO__,
  PATHDIRS__,
  POSIXBUILTINS__,
  SHINSTDIN__,
  SOURCETRACE__,
  ZP_OPT_COUNT__
};

/* Some upstream structs referenced via vendor headers rely on system types
 * like time_t, dev_t, ino_t, struct timespec before they are declared via
 * system headers. Ensure we include the standard headers early (without
 * editing vendor files). */
#include <sys/types.h>
#include <time.h>
