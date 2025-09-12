#pragma once
/* Lightweight shim to provide SIGCOUNT for builds that include signames.epro
 * and signals.h. Prefer the vendored zsh header if available; otherwise, use
 * a conservative fallback consistent with upstream defaults. */
#ifndef ZPMOD_SIGCOUNT_SHIM_H
#define ZPMOD_SIGCOUNT_SHIM_H

#if defined(__has_include)
#if __has_include("../../vendor/zsh/Src/sigcount.h")
#include "../../vendor/zsh/Src/sigcount.h"
#elif __has_include("../vendor/zsh/Src/sigcount.h")
#include "../vendor/zsh/Src/sigcount.h"
#elif __has_include("vendor/zsh/Src/sigcount.h")
#include "vendor/zsh/Src/sigcount.h"
#elif __has_include("Src/sigcount.h")
#include "Src/sigcount.h"
#endif
#endif

#ifndef SIGCOUNT
#define SIGCOUNT 31
#endif

#endif /* ZPMOD_SIGCOUNT_SHIM_H */
