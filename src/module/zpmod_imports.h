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

/* Parameter functions from params.c */
#ifndef mod_import_function
Param setaparam(char *s, char **aval);
char *getsparam(char *s);
char **getaparam(char *s);
Param setsparam(char *s, char *val);
void unsetparam(char *s);
#else
mod_import_function Param setaparam(char *s, char **aval);
mod_import_function char *getsparam(char *s);
mod_import_function char **getaparam(char *s);
mod_import_function Param setsparam(char *s, char *val);
mod_import_function void unsetparam(char *s);
#endif

/* Memory management functions from mem.c */
#ifndef mod_import_function
void *zalloc(size_t size);
void zfree(void *p, int sz);
void zsfree(char *p);
void *zrealloc(void *ptr, size_t size);
#else
mod_import_function void *zalloc(size_t size);
mod_import_function void zfree(void *p, int sz);
mod_import_function void zsfree(char *p);
mod_import_function void *zrealloc(void *ptr, size_t size);
#endif

/* String/metafication functions from utils.c */
#ifndef mod_import_function
char *metafy(char *buf, int len, int heap);
char *unmetafy(char *s, int *len);
void zwarnnam(const char *cmd, const char *fmt, ...);
#else
mod_import_function char *metafy(char *buf, int len, int heap);
mod_import_function char *unmetafy(char *s, int *len);
mod_import_function void zwarnnam(const char *cmd, const char *fmt, ...);
#endif

/* Option functions from options.c */
#ifndef mod_import_function
int optlookup(const char *name);
#else
mod_import_function int optlookup(const char *name);
#endif

/* Module feature functions from module.c */
#ifndef mod_import_function
char **featuresarray(Module m, Features f);
int handlefeatures(Module m, Features f, int **enables);
int setfeatureenables(Module m, Features f, int *e);
#else
mod_import_function char **featuresarray(Module m, Features f);
mod_import_function int handlefeatures(Module m, Features f, int **enables);
mod_import_function int setfeatureenables(Module m, Features f, int *e);
#endif

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

#endif /* ZPMOD_ZSH_IMPORTS_H */
