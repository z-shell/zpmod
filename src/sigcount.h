/* Fallback SIGCOUNT header for out-of-tree builds.
 * Upstream zsh generates this from signames.c. We provide a conservative
 * default that matches the baseline in vendored Src/signames.c. */
#ifndef ZSH_SIGCOUNT_FALLBACK_H
#define ZSH_SIGCOUNT_FALLBACK_H

#ifndef SIGCOUNT
#define SIGCOUNT 31
#endif

#endif /* ZSH_SIGCOUNT_FALLBACK_H */
