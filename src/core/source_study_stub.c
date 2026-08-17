/* SPDX-License-Identifier: MIT */
#include "zpmod.mdh"
#include "zpmod.pro"
/* Ensure this file compiles (guard against duplicate definition) */
#ifndef HAVE_ZPMOD_SOURCE_STUDY_CORE_STUB
#define HAVE_ZPMOD_SOURCE_STUDY_CORE_STUB 1
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
int zp_source_study_core(const char *nam, int report_count, int threshold_ms,
                         int clear_history) {
  (void)threshold_ms;
  (void)clear_history;
  (void)nam;
  if (report_count < 0) {
    return 1;
  }
  return 0; /* no-op */
}
/* Force a reference so the object file is not discarded even under LTO.
 * We create a local static function pointer with 'used' attribute so that
 * the symbol is considered referenced without triggering warnings. */
static int (*zp_source_study_core_anchor)(const char *, int, int, int)
    __attribute__((used)) = zp_source_study_core;
#endif
// NOLINTEND(bugprone-easily-swappable-parameters)
