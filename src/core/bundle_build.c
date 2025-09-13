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
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

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

/* Determine if bundle is up to date: all inputs older than bundle */
static int bb_is_fresh(const char *out_path, struct bb_vec *items) {
  struct stat st_out;
  if (stat(out_path, &st_out) != 0) {
    return 0;
  }
  for (size_t i = 0; i < items->size; i++) {
    if (items->items[i].mtime > st_out.st_mtime) {
      return 0;
    }
  }
  return 1;
}

int zp_bundle_build_core(char *nam, const char *from_dir, const char *out_path,
                         long max_kb) {
  if (!from_dir || !out_path) {
    zwarnnam(nam, "bundle-build: missing --from or --out");
    return 1;
  }
  struct stat st_root;
  if (stat(from_dir, &st_root) != 0 || !S_ISDIR(st_root.st_mode)) {
    zwarnnam(nam, "bundle-build: not a directory: %s", from_dir);
    return 1;
  }
  struct bb_vec items;
  bb_vec_init(&items);
  if (bb_collect(nam, from_dir, NULL, &items) != 0) {
    bb_vec_free(&items);
    return 1;
  }
  if (!items.size) {
    bb_vec_free(&items); /* nothing to do */
    return 0;
  }
  qsort(items.items, items.size, sizeof(struct bb_entry), bb_cmp);
  if (bb_is_fresh(out_path, &items)) { /* up to date */
    bb_vec_free(&items);
    return 0;
  }
  FILE *out = fopen(out_path, "w");
  if (!out) {
    zwarnnam(nam, "bundle-build: cannot open output %s: %s", out_path,
             strerror(errno));
    bb_vec_free(&items);
    return 1;
  }
  fprintf(out, "# bundle: generated by zpmod bundle-build\n");
  size_t limit_bytes = (max_kb > 0) ? (size_t)max_kb * 1024UL : 0;
  size_t used = 0;
  int truncated = 0;
  size_t included = 0;
  for (size_t i = 0; i < items.size; i++) {
    if (limit_bytes && used >= limit_bytes) {
      truncated = 1;
      break;
    }
    struct bb_entry *e = &items.items[i];
    size_t overhead = 32 + (strlen(e->rel) * 2);
    if (limit_bytes && (used + overhead) >= limit_bytes) {
      truncated = 1;
      break;
    }
    FILE *in = fopen(e->abs, "r");
    if (!in) {
      continue;
    }
    char buf[8192];
    size_t r;
    // NOLINTBEGIN(clang-analyzer-unix.Stream)
    while ((r = fread(buf, 1, sizeof(buf), in)) > 0) {
      if (limit_bytes && used + r >= limit_bytes) {
        size_t remain = limit_bytes - used;
        if (remain > 0) {
          (void)fwrite(buf, 1, remain, out);
          used += remain;
        }
        truncated = 1;
        break;
      }
      (void)fwrite(buf, 1, r, out);
      used += r;
    }
    // NOLINTEND(clang-analyzer-unix.Stream)
    if (ferror(in)) {
      /* Read error: stop processing this file gracefully */
      truncated = 1;
    }
    fclose(in);
    if (!truncated) {
      fprintf(out, "\n# END %s\n\n", e->rel);
      used += overhead;
    }
    included++;
    if (truncated) {
      break;
    }
  }
  if (truncated) {
    fprintf(out, "# NOTE: bundle truncated due to size limit\n");
  }
  fclose(out);
  fprintf(stdout, "bundle-build: %zu files -> %s (%zu bytes)%s\n", included,
          out_path, used, truncated ? " (truncated)" : "");
  fflush(stdout);
  bb_vec_free(&items);
  return 0;
}

// NOLINTEND(misc-include-cleaner)
