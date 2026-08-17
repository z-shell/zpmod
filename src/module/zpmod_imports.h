#pragma once
/* SPDX-License-Identifier: MIT */
/* Centralized imports for zsh-generated .epro prototypes and externs.
 * Maintenance:
 * - Only add a new .epro include here if you introduce usage of a symbol
 *   (function/variable/type) that causes a missing prototype/export at build.
 * - Prefer relying on zsh's zsh.mdh when available; this header is the
 *   out-of-tree fallback aggregator for required .epro declarations.
 */
#ifndef ZPMOD_ZSH_IMPORTS_H
#define ZPMOD_ZSH_IMPORTS_H

#if defined(__has_include)
#if __has_include("builtin.epro")
#include "builtin.epro"
#endif
#if __has_include("module.epro")
#include "module.epro"
#endif
#if __has_include("params.epro")
#include "params.epro"
#endif
#if __has_include("mem.epro")
#include "mem.epro"
#endif
#if __has_include("string.epro")
#include "string.epro"
#endif
#if __has_include("hashtable.epro")
#include "hashtable.epro"
#endif
#if __has_include("options.epro")
#include "options.epro"
#endif
#if __has_include("parse.epro")
#include "parse.epro"
#endif
#if __has_include("input.epro")
#include "input.epro"
#endif
#if __has_include("jobs.epro")
#include "jobs.epro"
#endif
#if __has_include("prompt.epro")
#include "prompt.epro"
#endif
#if __has_include("loop.epro")
#include "loop.epro"
#endif
#if __has_include("init.epro")
#include "init.epro"
#endif
#if __has_include("utils.epro")
#include "utils.epro"
#endif
#if __has_include("signals.epro")
#include "signals.epro"
#endif
#if __has_include("signames.epro")
#include "signames.epro"
#endif
#endif /* __has_include */

/* Fallback declarations for out-of-tree builds when zsh-generated .epro
 * headers are not available in the include path. These are minimal
 * prototypes sufficient for compilation; symbols are resolved at runtime
 * by the hosting zsh.
 */
#include <stddef.h>

#if !defined(__has_include) || !__has_include("mem.epro")
void *zalloc(size_t size);
void zfree(void *ptr, size_t size);
void zsfree(char *ptr);
void *zrealloc(void *ptr, size_t size);
#endif

#if !defined(__has_include) || !__has_include("string.epro")
char *ztrdup(const char *s);
char *dupstring(const char *s);
#endif

#if !defined(__has_include) || !__has_include("utils.epro")
char *metafy(char *s, int len, int how);
char *unmetafy(char *s, int *len);
#endif

#if !defined(__has_include) || !__has_include("params.epro")
char *getsparam(const char *name);
char **getaparam(const char *name);
void setsparam(const char *name, char *value);
void setaparam(const char *name, char **value);
void unsetparam(const char *name);
extern HashTable paramtab;
#endif

#if !defined(__has_include) || !__has_include("hashtable.epro")
HashNode gethashnode2(HashTable ht, const char *name);
#endif

#if !defined(__has_include) || !__has_include("options.epro")
int optlookup(const char *name);
#endif

#if !defined(__has_include) || !__has_include("builtin.epro")
void zwarnnam(const char *cmd, const char *fmt, ...);
#endif

#if !defined(__has_include) || !__has_include("module.epro")
/* Note: "Features" is a pointer typedef (struct features *). The real
 * prototypes take a Features (pointer), not a pointer-to-pointer. */
char **featuresarray(Module m, Features features);
int handlefeatures(Module m, Features features, int **enables);
int setfeatureenables(Module m, Features features, int **enables);
#endif

#endif /* ZPMOD_ZSH_IMPORTS_H */
