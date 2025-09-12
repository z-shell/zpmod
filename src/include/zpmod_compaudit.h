#pragma once
/* SPDX-License-Identifier: MIT */
#ifndef ZPMOD_COMPAUDIT_H
#define ZPMOD_COMPAUDIT_H
/**
 * @file zpmod_compaudit.h
 * @brief Interface for cached compaudit security verdicts.
 *
 * Phase 2 (scaffolding): Provides entrypoint for `zpmod compaudit-cache`.
 * Future implementation goals:
 *  - Cache directory ownership+permission verdicts equivalent to native compaudit
 *  - Invalidate on metadata changes (mtime/ctime, uid/gid, mode)
 *  - Store cache in XDG cache dir (~/.cache/zpmod) with permission checks
 *  - Provide `--rebuild` to force regeneration, `--show` to print current cache
 */
int zp_compaudit_cache_core(char *nam, int rebuild, int show, int json);

#endif /* ZPMOD_COMPAUDIT_H */
