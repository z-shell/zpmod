/* SPDX-License-Identifier: MIT */
/* Centralized imports for zsh-generated .epro prototypes and externs. */
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

#endif /* ZPMOD_ZSH_IMPORTS_H */
