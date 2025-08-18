/* SPDX-License-Identifier: MIT */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_compat.h"

static int zp_opt_for_zsh_version[256] = {0};

struct zp_option_name {
  const char *name;
  int enum_val;
};

static struct zp_option_name zp_options[] = {};

void zp_setup_options_table(void) {
  for (int i = 0; zp_options[i].name != NULL; ++i) {
    int e = zp_options[i].enum_val;
    if (e < 0 || e >= (int)(sizeof(zp_opt_for_zsh_version) /
                            sizeof(zp_opt_for_zsh_version[0])))
      continue;
    int optno = optlookup(zp_options[i].name);
    if (optno >= 0)
      zp_opt_for_zsh_version[e] = optno;
  }
}

int zp_conv_opt(int zp_opt_num) {
  int sign = (zp_opt_num >= 0) ? 1 : -1;
  int idx = sign * zp_opt_num;
  if (idx < 0 || idx >= (int)(sizeof(zp_opt_for_zsh_version) /
                              sizeof(zp_opt_for_zsh_version[0])))
    return 0;
  return sign * zp_opt_for_zsh_version[idx];
}
