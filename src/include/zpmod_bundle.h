#pragma once
/* SPDX-License-Identifier: MIT */
#ifndef ZPMOD_BUNDLE_H
#define ZPMOD_BUNDLE_H
/**
 * @file zpmod_bundle.h
 * @brief Startup bundle builder interface.
 *
 * Phase 2 scaffold: `zpmod bundle-build --from <dir> --out <bundle.zsh> [--max
 * <KB>]` will concat *.zsh / *.plugin.zsh scripts deterministically (sorted
 * lexical) up to an optional size cap, writing a single bundle suitable for
 * zcompile.
 */
int zp_bundle_build_core(char *nam, const char *from_dir, const char *out_path,
                         long max_kb);

#endif /* ZPMOD_BUNDLE_H */
