#pragma once
/* SPDX-License-Identifier: MIT */
#ifndef ZPMOD_REHASH_H
#define ZPMOD_REHASH_H
/**
 * @file zpmod_rehash.h
 * @brief Incremental PATH diff rehash entrypoint.
 *
 * Phase 2 scaffold: `zpmod rehash-diff` will rehash only new/changed PATH
 * segments versus a cached snapshot of prior directories + mtimes.
 */
int zp_rehash_diff_core(char *nam);

#endif /* ZPMOD_REHASH_H */
