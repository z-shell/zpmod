/* SPDX-License-Identifier: MIT */
/**
 * @file module.c
 * @brief zsh module glue: builtins table, features, and lifecycle hooks.
 */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_vendor_shims.h"
#include "zpmod_builtins.h"
#include "zpmod_version.h"

/* Declare builtin handlers implemented in other TUs */
int bin_zppathstat(char *nam, char **argv, Options ops, int func);
int bin_zpdirlist(char *nam, char **argv, Options ops, int func);
int bin_zpreadfile(char *nam, char **argv, Options ops, int func);


/* Unified builtin table (static, like original) */
static struct builtin bintab[] = {
#ifdef ZPMOD_HAVE_SOURCE_STUDY
  BUILTIN("custom_dot", 0, bin_custom_dot, 1, -1, 0, NULL, NULL),
#endif
  BUILTIN("readarray", 0, bin_readarray, 1, 1, 0, "d:n:O:s:tu:C:c:h", NULL),
  BUILTIN("zppathstat", 0, bin_zppathstat, 2, 2, 0, "Lf:", NULL),
  BUILTIN("zpdirlist", 0, bin_zpdirlist, 2, 2, 0, "adf", NULL),
  BUILTIN("zpreadfile", 0, bin_zpreadfile, 2, 2, 0, "md:0", NULL),
  BUILTIN("zpmod", 0, bin_zpmod, 0, -1, 0, "hV", NULL),
};

static struct features module_features = {
    bintab, (int)(sizeof(bintab) / sizeof(*bintab)), NULL, 0, NULL, 0, NULL, 0,
    0};

/** Module setup: initialize option mapping and install source overrides. */
int setup_(UNUSED(Module m)) {
  extern void zp_setup_options_table(void);
  zp_setup_options_table();
#ifdef ZPMOD_HAVE_SOURCE_STUDY
  extern void zp_source_setup_overrides(void);
  zp_source_setup_overrides();
#endif
  return 0;
}

/** Provide feature list (builtins) to zsh. */
int features_(Module m, char ***features) {
  *features = featuresarray(m, &module_features);
  return 0;
}
/** Enable/disable builtins as requested by the shell. */
int enables_(Module m, int **enables) {
  return handlefeatures(m, &module_features, enables);
}
/** Optional early boot hook (unused). */
int boot_(UNUSED(Module m)) { return 0; }
/** Cleanup features when unloading. */
int cleanup_(Module m) { return setfeatureenables(m, &module_features, NULL); }
/** Finalize module: restore original source handlers. */
int finish_(UNUSED(Module m)) {
#ifdef ZPMOD_HAVE_SOURCE_STUDY
  extern void zp_source_restore_overrides(void);
  zp_source_restore_overrides();
#endif
  return 0;
}
