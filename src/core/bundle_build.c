/* SPDX-License-Identifier: MIT */
/**
 * @file bundle_build.c
 * @brief Startup bundle builder (initial functional implementation).
 */
// NOLINTBEGIN(misc-include-cleaner)
/* Canonical module header ordering: include gateway first
 * (aggregates vendor .epro via zpmod_imports.h as needed).
 */
#include "zpmod.mdh"
#include "zpmod.pro"
/* System headers after gateway */
#include "zpmod_bundle.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

/* Portable fallback for platforms that don't define PATH_MAX in <limits.h>. */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Accept these file suffixes (exact) */
static int has_ext(const char *name) {
  size_t len = strlen(name);
  if (len >= 4 && strcmp(name + len - 4, ".zsh") == 0) {
    return 1;
  }
  if (len >= 12 && strcmp(name + len - 12, ".plugin.zsh") == 0) {
    return 1;
  }
  return 0;
}

struct bb_entry {
  char *rel;
  char *abs;
  off_t size;
  time_t mtime;
};
struct bb_vec {
  struct bb_entry *items;
  size_t size;
  size_t cap;
};
static void bb_vec_init(struct bb_vec *v) {
  v->items = NULL;
  v->size = 0;
  v->cap = 0;
}
static int bb_vec_push(struct bb_vec *v, struct bb_entry *e) {
  if (v->size == v->cap) {
    size_t nc = v->cap ? v->cap * 2 : 32;
    void *nb = zrealloc(v->items, nc * sizeof(struct bb_entry));
    if (!nb) {
      return 1;
    }
    v->items = (struct bb_entry *)nb;
    v->cap = nc;
  }
  v->items[v->size++] = *e;
  return 0;
}
static void bb_vec_free(struct bb_vec *v) {
  if (!v) {
    return;
  }
  for (size_t i = 0; i < v->size; i++) {
    if (v->items[i].rel) {
      zsfree(v->items[i].rel);
    }
    if (v->items[i].abs) {
      zsfree(v->items[i].abs);
    }
  }
  if (v->items) {
    zfree(v->items, v->cap * sizeof(struct bb_entry));
  }
  v->items = NULL;
  v->size = v->cap = 0;
}

/* Recursive collection */
// NOLINTBEGIN(misc-no-recursion)
static int bb_collect(char *nam, const char *root, const char *sub,
                      struct bb_vec *out) {
  char path[PATH_MAX];
  if (sub && *sub) {
    snprintf(path, sizeof(path), "%s/%s", root, sub);
  } else {
    snprintf(path, sizeof(path), "%s", root);
  }
  DIR *d = opendir(path);
  if (!d) {
    return 0;
  }
  struct dirent *de;
  while ((de = readdir(d)) != NULL) {
    if (de->d_name[0] == '.') {
      continue;
    }
    char rel[PATH_MAX];
    if (sub && *sub) {
      snprintf(rel, sizeof(rel), "%s/%s", sub, de->d_name);
    } else {
      snprintf(rel, sizeof(rel), "%s", de->d_name);
    }
    char abs[PATH_MAX];
    snprintf(abs, sizeof(abs), "%s/%s", root, rel);
    struct stat st;
    if (stat(abs, &st) != 0) {
      continue;
    }
    if (S_ISDIR(st.st_mode)) {
      bb_collect(nam, root, rel, out);
      continue;
    }
    if (!S_ISREG(st.st_mode)) {
      continue;
    }
    if (!has_ext(de->d_name)) {
      continue;
    }
    size_t rlen = strlen(rel) + 1;
    size_t alen = strlen(abs) + 1;
    char *rdup = (char *)zalloc(rlen);
    char *adup = (char *)zalloc(alen);
    if (!rdup || !adup) {
      if (rdup) {
        zsfree(rdup);
      }
      if (adup) {
        zsfree(adup);
      }
      closedir(d);
      return 1;
    }
    memcpy(rdup, rel, rlen);
    memcpy(adup, abs, alen);
    struct bb_entry e;
    e.rel = rdup;
    e.abs = adup;
    e.size = st.st_size;
    e.mtime = st.st_mtime;
    if (bb_vec_push(out, &e)) {
      zsfree(rdup);
      zsfree(adup);
      closedir(d);
      return 1;
    }
  }
  closedir(d);
  return 0;
}
// NOLINTEND(misc-no-recursion)

/* Lexicographic sort (C locale) */
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int bb_cmp(const void *a, const void *b) {
  const struct bb_entry *ea = a;
  const struct bb_entry *eb = b;
  return strcmp(ea->rel, eb->rel);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTEND(misc-include-cleaner)
