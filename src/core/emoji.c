/* SPDX-License-Identifier: MIT */
#include "zpmod.mdh"
#include "zpmod.pro"
#include <locale.h>
#include <unistd.h>
#if defined(__has_include)
#if __has_include(<langinfo.h>)
#include <langinfo.h>
#define ZPMOD_HAVE_LANGINFO 1
#endif
#endif
#include "zpmod_emoji.h"

static int s_cached = -1;

int zp_icons_enabled(void) {
  if (s_cached != -1)
    return s_cached;
  const char *env = getsparam("ZPMOD_ICONS");
  if (env) {
    if (!strcmp(env, "0") || !strcmp(env, "false") || !strcmp(env, "off"))
      return (s_cached = 0);
    if (!strcmp(env, "1") || !strcmp(env, "true") || !strcmp(env, "on"))
      return (s_cached = 1);
  }
  if (!isatty(STDOUT_FILENO))
    return (s_cached = 0);
  setlocale(LC_ALL, "");
#ifdef ZPMOD_HAVE_LANGINFO
  const char *cs = nl_langinfo(CODESET);
  if (cs && (strstr(cs, "UTF-8") || strstr(cs, "utf8") || strstr(cs, "UTF8")))
    return (s_cached = 1);
#else
  const char *lc = getenv("LC_ALL");
  if (!lc)
    lc = getenv("LANG");
  if (lc && (strstr(lc, "UTF-8") || strstr(lc, "utf8") || strstr(lc, "UTF8")))
    return (s_cached = 1);
#endif
  return (s_cached = 0);
}

const char *zp_icon(const char *s) { return zp_icons_enabled() ? s : ""; }
