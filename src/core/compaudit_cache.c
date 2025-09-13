/* SPDX-License-Identifier: MIT */
/**
 * @file compaudit_cache.c
 * @brief Cached security verdicts for completion directories (initial slice).
 *
 * Format (line oriented; header + entries):
 *   version:3\n
 *   <path>\t<verdict>\t<mode_octal>\t<uid>\t<gid>\t<mtime>\t<ctime>\t<parent_insecure>\t<zwc_insecure>\n
 *     verdict: 0 = secure, 1 = insecure (mirrors compaudit style verdict)
 *
 * Rebuild conditions (current slice):
 *  - --rebuild flag
 *  - cache file missing
 *  - version mismatch
 *  - any directory metadata (mode/uid/gid/ctime/mtime) differs from cache
 */
// NOLINTBEGIN(misc-include-cleaner)
/* Canonical module header ordering: include gateway first */
#include "zpmod.mdh"
#include "zpmod.pro"
/* System headers after gateway */
#include "zpmod_compaudit.h"
#include "zpmod_emoji.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define ZP_COMPAUDIT_CACHE_SUBDIR "zpmod"
#define ZP_COMPAUDIT_CACHE_FILE "compaudit_v3.zcache"

/* -------- Helpers -------- */

/* Choose XDG cache dir or fallback to ~/.cache; return newly allocated string.
 */
static char *zp_cc_base_cache_dir(void) {
  const char *xdg = getsparam("XDG_CACHE_HOME");
  const char *home = getsparam("HOME");
  const char *base = (xdg && *xdg) ? xdg : NULL;
  if (!base && home && *home) {
    size_t len = strlen(home) + sizeof("/.cache");
    char *p = (char *)zalloc(len);
    if (!p) {
      return NULL;
    }
    strcpy(p, home);
    strcat(p, "/.cache");
    return p; /* caller frees */
  }
  if (base) {
    return ztrdup(base);
  }
  return NULL;
}

/* Ensure zpmod cache directory exists (0700). Returns allocated full dir path
 * or NULL. */
static char *zp_cc_ensure_dir(char *nam) {
  char *base = zp_cc_base_cache_dir();
  if (!base) {
    zwarnnam(nam, "%scompaudit-cache: cannot resolve cache directory (no HOME)",
             zp_icon("❌ "));
    return NULL;
  }
  /* append /zpmod */
  size_t len = strlen(base) + 1 + strlen(ZP_COMPAUDIT_CACHE_SUBDIR) + 1;
  char *full = (char *)zalloc(len);
  if (!full) {
    zsfree(base);
    return NULL;
  }
  sprintf(full, "%s/%s", base, ZP_COMPAUDIT_CACHE_SUBDIR);
  struct stat st;
  if (stat(full, &st) != 0) {
    if (mkdir(full, 0700) != 0) {
      zwarnnam(nam, "%scompaudit-cache: cannot create %s: %s", zp_icon("❌ "),
               full, strerror(errno));
      zsfree(base);
      zsfree(full);
      return NULL;
    }
  } else {
    if (!S_ISDIR(st.st_mode)) {
      zwarnnam(nam, "%scompaudit-cache: %s exists and is not a directory",
               zp_icon("❌ "), full);
      zsfree(base);
      zsfree(full);
      return NULL;
    }
  }
  zsfree(base);
  return full;
}

static char *zp_cc_cache_file_path(char *nam) {
  char *dir = zp_cc_ensure_dir(nam);
  if (!dir) {
    return NULL;
  }
  size_t len = strlen(dir) + 1 + strlen(ZP_COMPAUDIT_CACHE_FILE) + 1;
  char *p = (char *)zalloc(len);
  if (!p) {
    zsfree(dir);
    return NULL;
  }
  sprintf(p, "%s/%s", dir, ZP_COMPAUDIT_CACHE_FILE);
  zsfree(dir);
  return p;
}

struct zp_cc_entry {
  char *path;
  int verdict;
  mode_t mode;
  uid_t uid;
  gid_t gid;
  time_t mtime;
  time_t ctime;
  int parent_insecure;
  int zwc_insecure;
};

/* forward */
static int parent_insecure_path(const char *path, uid_t self);

/* Very small vector for entries */
struct zp_cc_vec {
  struct zp_cc_entry *items;
  size_t size;
  size_t cap;
};
static void zp_cc_vec_init(struct zp_cc_vec *v) {
  v->items = NULL;
  v->size = 0;
  v->cap = 0;
}
static int zp_cc_vec_push(struct zp_cc_vec *v, struct zp_cc_entry *e) {
  if (v->size == v->cap) {
    size_t ncap = v->cap ? v->cap * 2 : 8;
    void *nbuf = zrealloc(v->items, ncap * sizeof(struct zp_cc_entry));
    if (!nbuf) {
      return 1;
    }
    v->items = (struct zp_cc_entry *)nbuf;
    v->cap = ncap;
  }
  v->items[v->size++] = *e;
  return 0;
}
static void zp_cc_vec_free(struct zp_cc_vec *v) {
  if (!v) {
    return;
  }
  for (size_t i = 0; i < v->size; i++) {
    if (v->items[i].path) {
      zsfree(v->items[i].path);
    }
  }
  if (v->items) {
    zfree(v->items, v->cap * sizeof(struct zp_cc_entry));
  }
  v->items = NULL;
  v->size = v->cap = 0;
}

/*
 * Security verdict parity slice:
 *  - Treat directory insecure if:
 *      * owned by someone other than allowed owners (root, EUID, zsh exe owner
 * if detectable) AND (world-writable and not sticky) OR (group-writable and not
 * sticky)
 *      * OR it is a symlink whose final target satisfies above (stat() already
 * resolves)
 *  - Sticky bit (S_ISVTX) on a world/group writable dir neutralizes that
 * writable bit (like /tmp semantics)
 *  - Root/EUID/exe-owner owned dirs are considered secure unless world-writable
 * without sticky (defensive)
 */
static uid_t zp_cached_exe_owner = (uid_t)-1;
static void zp_cc_discover_exe_owner(void) {
  if (zp_cached_exe_owner != (uid_t)-1) {
    return;
  }
  const char *candidates[] = {"/proc/self/exe", NULL};
  for (int cand_index = 0; candidates[cand_index]; ++cand_index) {
    struct stat st;
    if (stat(candidates[cand_index], &st) == 0) {
      zp_cached_exe_owner = st.st_uid;
      return;
    }
  }
  zp_cached_exe_owner = 0; /* fallback root */
}
static int zp_cc_owner_allowed(uid_t uid, uid_t self) {
  if (uid == 0 || uid == self) {
    return 1;
  }
  if (zp_cached_exe_owner != (uid_t)-1 && uid == zp_cached_exe_owner) {
    return 1;
  }
  return 0;
}
static int zp_cc_is_insecure(struct stat *st, uid_t self) {
  if (!st) {
    return 1;
  }
  zp_cc_discover_exe_owner();
  int sticky = (st->st_mode & S_ISVTX) ? 1 : 0;
  int world_w = (st->st_mode & S_IWOTH) ? 1 : 0;
  int group_w = (st->st_mode & S_IWGRP) ? 1 : 0;
  int allowed_owner = zp_cc_owner_allowed(st->st_uid, self);
  if (!allowed_owner) {
    if ((world_w && !sticky) || (group_w && !sticky)) {
      return 1;
    }
  } else {
    /* even if owner allowed, treat truly open world-writable non-sticky as
     * insecure */
    if (world_w && !sticky) {
      return 1;
    }
  }
  return 0;
}

/* Collect target directories (initial slice: $fpath elements). */
static void zp_cc_collect_dirs(struct zp_cc_vec *out) {
  char **arr = getaparam("fpath");
  if (!arr) {
    return;
  }
  for (char **fpath_it = arr; *fpath_it; ++fpath_it) {
    if (!**fpath_it) {
      continue;
    }
    size_t len = strlen(*fpath_it) + 1;
    char *dup = (char *)zalloc(len);
    if (!dup) {
      break;
    }
    memcpy(dup, *fpath_it, len);
    struct zp_cc_entry e;
    e.path = dup;
    e.verdict = 0;
    e.mode = 0;
    e.uid = 0;
    e.gid = 0;
    e.mtime = 0;
    e.ctime = 0;
    if (zp_cc_vec_push(out, &e)) {
      zsfree(dup);
      break;
    }
  }
}

/* Load cache into a simple vector; returns 0 success, 1 error. */
static int zp_cc_load_cache(const char *path, struct zp_cc_vec *out) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    return 1;
  }
  char line[4096];
  if (!fgets(line, sizeof(line), fp)) {
    fclose(fp);
    return 1;
  }
  if (strncmp(line, "version:3", 9) != 0) {
    fclose(fp);
    return 1;
  }
  while (fgets(line, sizeof(line), fp)) {
    if (line[0] == '\n' || line[0] == '#') {
      continue;
    }
    char *nl = strchr(line, '\n');
    if (nl) {
      *nl = '\0';
    }
    char *save = NULL;
    char *tok = strtok_r(line, "\t", &save);
    if (!tok) {
      continue;
    }
    char *path_dup = ztrdup(tok);
    int fld = 0;
    long fields[8]; /* verdict, mode(octal), uid, gid, mtime, ctime,
                       parent_insecure, zwc_insecure */
    while (fld < 8 && (tok = strtok_r(NULL, "\t", &save))) {
      int base = (fld == 1) ? 8 : 10;
      fields[fld] = strtol(tok, NULL, base);
      fld++;
    }
    if (fld != 8) {
      if (path_dup) {
        zsfree(path_dup);
      }
      continue;
    }
    struct zp_cc_entry e;
    e.path = path_dup;
    e.verdict = (int)fields[0];
    e.mode = (mode_t)fields[1];
    e.uid = (uid_t)fields[2];
    e.gid = (gid_t)fields[3];
    e.mtime = (time_t)fields[4];
    e.ctime = (time_t)fields[5];
    e.parent_insecure = (int)fields[6];
    e.zwc_insecure = (int)fields[7];
    if (zp_cc_vec_push(out, &e)) {
      if (path_dup) {
        zsfree(path_dup);
      }
      break;
    }
  }
  fclose(fp);
  return 0;
}

/* Validate cache: any metadata difference triggers invalid (return 1). */
static int zp_cc_validate_cache(struct zp_cc_vec *cache) {
  uid_t self = geteuid();
  for (size_t i = 0; i < cache->size; i++) {
    struct stat st;
    if (stat(cache->items[i].path, &st) != 0) {
      return 1;
    }
    if (cache->items[i].uid != st.st_uid || cache->items[i].gid != st.st_gid) {
      return 1;
    }
    if (cache->items[i].mode != (st.st_mode & 07777)) {
      return 1;
    }
    if (cache->items[i].ctime != st.st_ctime ||
        cache->items[i].mtime != st.st_mtime) {
      return 1;
    }
    int v = zp_cc_is_insecure(&st, self);
    int p_insec = parent_insecure_path(cache->items[i].path, self);
    if (!v && p_insec) {
      v = 1;
    }
    if (v != cache->items[i].verdict) {
      return 1;
    }
  }
  return 0;
}

/* Incremental update: refresh only changed entries and add new fpath dirs. */
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int zp_cc_incremental_update(char *nam, char *cache_path,
                                    struct zp_cc_vec *entries,
                                    int *out_insecure, int *out_secure) {
  uid_t self = geteuid();
  /* Mark presence of existing entries */
  for (size_t i = 0; i < entries->size;
       i++) { /* reuse parent_insecure as temp mark? no; allocate bitmap
                 dynamically */
  }
  /* Gather fpath and add any missing directories */
  char **arr = getaparam("fpath");
  if (arr) {
    for (char **fpath_it2 = arr; *fpath_it2; ++fpath_it2) {
      if (!**fpath_it2) {
        continue;
      }
      int found = 0;
      for (size_t ent_idx = 0; ent_idx < entries->size; ent_idx++) {
        if (strcmp(entries->items[ent_idx].path, *fpath_it2) == 0) {
          found = 1;
          break;
        }
      }
      if (!found) {
        size_t len = strlen(*fpath_it2) + 1;
        char *dup = (char *)zalloc(len);
        if (!dup) {
          break;
        }
        memcpy(dup, *fpath_it2, len);
        struct zp_cc_entry e;
        e.path = dup;
        e.verdict = 0;
        e.mode = 0;
        e.uid = 0;
        e.gid = 0;
        e.mtime = 0;
        e.ctime = 0;
        e.parent_insecure = 0;
        e.zwc_insecure = 0; /* will fill */
        if (zp_cc_vec_push(entries, &e)) {
          zsfree(dup);
          break;
        }
      }
    }
  }
  int insecure = 0;
  int secure = 0;
  FILE *fp = fopen(cache_path, "w");
  if (!fp) {
    zwarnnam(nam, "%scompaudit-cache: cannot update %s: %s", zp_icon("❌ "),
             cache_path, strerror(errno));
    return 1;
  }
  fchmod(fileno(fp), 0600);
  fprintf(fp, "version:3\n");
  for (size_t i = 0; i < entries->size; i++) {
    struct stat st;
    if (stat(entries->items[i].path, &st) != 0) {
      continue;
    }
    int verdict = zp_cc_is_insecure(&st, self);
    int p_insec = parent_insecure_path(entries->items[i].path, self);
    if (!verdict && p_insec) {
      verdict = 1;
    }
    entries->items[i].verdict = verdict;
    entries->items[i].mode = (st.st_mode & 07777);
    entries->items[i].uid = st.st_uid;
    entries->items[i].gid = st.st_gid;
    entries->items[i].mtime = st.st_mtime;
    entries->items[i].ctime = st.st_ctime;
    entries->items[i].parent_insecure = p_insec;
    if (verdict) {
      insecure++;
    } else {
      secure++;
    }
    int zwc_insec = entries->items[i].zwc_insecure;
    if (verdict && !zwc_insec) {
      DIR *d = opendir(entries->items[i].path);
      if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
          if (de->d_name[0] == '.') {
            continue;
          }
          size_t nlen = strlen(de->d_name);
          if (nlen >= 4 && strcmp(de->d_name + nlen - 4, ".zwc") == 0) {
            char full[PATH_MAX];
            if (snprintf(full, sizeof(full), "%s/%s", entries->items[i].path,
                         de->d_name) < (int)sizeof(full)) {
              struct stat stf;
              if (stat(full, &stf) == 0 && (stf.st_mode & 022)) {
                zwc_insec = 1;
                break;
              }
            }
          }
        }
        closedir(d);
      }
    }
    entries->items[i].zwc_insecure = zwc_insec;
    fprintf(fp, "%s\t%d\t%o\t%lu\t%lu\t%ld\t%ld\t%d\t%d\n",
            entries->items[i].path, verdict, (unsigned)(st.st_mode & 07777),
            (unsigned long)st.st_uid, (unsigned long)st.st_gid,
            (long)st.st_mtime, (long)st.st_ctime, p_insec, zwc_insec);
  }
  fclose(fp);
  if (out_insecure) {
    *out_insecure = insecure;
  }
  if (out_secure) {
    *out_secure = secure;
  }
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

/* Rebuild cache: stat each target dir and write new file. */
static int parent_insecure_path(const char *path, uid_t self) {
  char buf[PATH_MAX];
  size_t len = strlen(path);
  if (len >= sizeof(buf)) {
    return 0;
  }
  memcpy(buf, path, len + 1);
  while (1) {
    char *slash = strrchr(buf, '/');
    if (!slash || slash == buf) {
      break;
    }
    *slash = '\0';
    struct stat st;
    if (stat(buf, &st) != 0) {
      continue;
    }
    if (zp_cc_is_insecure(&st, self)) {
      return 1;
    }
  }
  return 0;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int zp_cc_rebuild(char *nam, const char *file_path,
                         struct zp_cc_vec *targets, int *out_insecure,
                         int *out_secure) {
  uid_t self = geteuid();
  int insecure = 0;
  int secure = 0;
  FILE *fp = fopen(file_path, "w");
  if (!fp) {
    zwarnnam(nam, "%scompaudit-cache: cannot write %s: %s", zp_icon("❌ "),
             file_path, strerror(errno));
    return 1;
  }
  fchmod(fileno(fp), 0600);
  fprintf(fp, "version:3\n");
  for (size_t i = 0; i < targets->size; i++) {
    struct stat st;
    if (stat(targets->items[i].path, &st) != 0) {
      continue; /* skip missing */
    }
    int verdict = zp_cc_is_insecure(&st, self);
    int p_insec = parent_insecure_path(targets->items[i].path, self);
    if (!verdict && p_insec) {
      verdict = 1;
    }
    if (verdict) {
      insecure++;
    } else {
      secure++;
    }
    int zwc_insec = 0;
    if (verdict) {
      DIR *d = opendir(targets->items[i].path);
      if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
          if (de->d_name[0] == '.') {
            continue;
          }
          size_t nlen = strlen(de->d_name);
          if (nlen >= 4 && strcmp(de->d_name + nlen - 4, ".zwc") == 0) {
            char full[PATH_MAX];
            if (snprintf(full, sizeof(full), "%s/%s", targets->items[i].path,
                         de->d_name) < (int)sizeof(full)) {
              struct stat stf;
              if (stat(full, &stf) == 0 && (stf.st_mode & 022)) {
                zwc_insec = 1;
                break;
              }
            }
          }
        }
        closedir(d);
      }
    }
    fprintf(fp, "%s\t%d\t%o\t%lu\t%lu\t%ld\t%ld\t%d\t%d\n",
            targets->items[i].path, verdict, (unsigned)(st.st_mode & 07777),
            (unsigned long)st.st_uid, (unsigned long)st.st_gid,
            (long)st.st_mtime, (long)st.st_ctime, p_insec, zwc_insec);
  }
  fclose(fp);
  if (out_insecure) {
    *out_insecure = insecure;
  }
  if (out_secure) {
    *out_secure = secure;
  }
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTBEGIN(readability-function-cognitive-complexity,bugprone-easily-swappable-parameters)
int zp_compaudit_cache_core(char *nam, int rebuild, int show, int json) {
  int ret = 0;
  char *cache_path = zp_cc_cache_file_path(nam);
  if (!cache_path) {
    return 1;
  }
  /* Migration: if v3 missing but legacy v2 exists, force rebuild and remove v2
   */
  if (!rebuild) {
    struct stat st_new;
    if (stat(cache_path, &st_new) != 0) {
      const char *needle = "_v3.zcache";
      const char *rep = "_v2.zcache";
      char *p = strstr(cache_path, needle);
      if (p) {
        size_t prefix = (size_t)(p - cache_path);
        size_t v2len = prefix + strlen(rep) + 1;
        char *v2path = (char *)zalloc(v2len);
        if (v2path) {
          memcpy(v2path, cache_path, prefix);
          strcpy(v2path + prefix, rep);
          struct stat st_old;
          if (stat(v2path, &st_old) == 0) {
            rebuild = 1;
            unlink(v2path); /* best-effort removal */
          }
          zsfree(v2path);
        }
      }
    }
  }

  struct zp_cc_vec cache_entries;
  zp_cc_vec_init(&cache_entries);
  int have_cache = 0;
  int insecure = 0;
  int secure = 0; /* may be filled by incremental update */
  struct stat cst;
  if (!rebuild && stat(cache_path, &cst) == 0) {
    if ((cst.st_mode & S_IWOTH) || (cst.st_mode & S_IWGRP)) {
      rebuild = 1;
    }
  }
  if (!rebuild) {
    if (zp_cc_load_cache(cache_path, &cache_entries) == 0) {
      have_cache = 1;
      if (zp_cc_validate_cache(&cache_entries) != 0) {
        /* Perform incremental update instead of discarding entirely */
        if (zp_cc_incremental_update(nam, cache_path, &cache_entries, &insecure,
                                     &secure) == 0) {
          /* reload fresh for show/json uniformity */
          zp_cc_vec_free(&cache_entries);
          zp_cc_vec_init(&cache_entries);
          zp_cc_load_cache(cache_path, &cache_entries);
          have_cache = 1;
        } else {
          zp_cc_vec_free(&cache_entries);
          have_cache = 0; /* fallback to full rebuild */
        }
      }
    }
  }
  if (insecure || secure) { /* already populated by incremental update path */
  }
  if (!have_cache) {
    /* Need targets to rebuild */
    struct zp_cc_vec targets;
    zp_cc_vec_init(&targets);
    zp_cc_collect_dirs(&targets);
    if (zp_cc_rebuild(nam, cache_path, &targets, &insecure, &secure) != 0) {
      zp_cc_vec_free(&targets);
      zsfree(cache_path);
      return 1;
    }
    zp_cc_vec_free(&targets);
    /* load freshly written for uniformity if show requested */
    if (show || json) {
      zp_cc_load_cache(cache_path, &cache_entries);
    }
  } else {
    /* derive counts from existing entries */
    for (size_t i = 0; i < cache_entries.size; i++) {
      if (cache_entries.items[i].verdict) {
        insecure++;
      } else {
        secure++;
      }
    }
  }
  if (json) {
    fprintf(stdout, "{\"insecure\":%d,\"secure\":%d,\"dirs\":[", insecure,
            secure);
    uid_t self = geteuid();
    for (size_t i = 0; i < cache_entries.size; i++) {
      struct zp_cc_entry *e = &cache_entries.items[i];
      int reason_dir = 0;
      int reason_ancestor = 0;
      int reason_zwc = 0;
      if (e->verdict) {
        /* directory perms */
        struct stat st_dir;
        if (stat(e->path, &st_dir) == 0) {
          if (zp_cc_is_insecure(&st_dir, self)) {
            reason_dir = 1;
          }
        }
        if (e->parent_insecure) {
          reason_ancestor = 1;
        }
        if (e->zwc_insecure) {
          reason_zwc = 1;
        }
      }
      fprintf(stdout, "%s{\"path\":\"", i ? "," : "");
      const char *p = e->path;
      for (; *p; p++) {
        if (*p == '\\' || *p == '\"') {
          fputc('\\', stdout);
        }
        fputc(*p, stdout);
      }
      fprintf(stdout, "\",\"verdict\":%d,\"parent_insecure\":%d,\"reasons\":[",
              e->verdict, e->parent_insecure);
      int first = 1;
      if (reason_dir) {
        fprintf(stdout, "\"dir_perms\"");
        first = 0;
      }
      if (reason_ancestor) {
        fprintf(stdout, "%s\"ancestor_perms\"", first ? "" : " ,");
        first = 0;
      }
      if (reason_zwc) {
        fprintf(stdout, "%s\"zwc_perms\"", first ? "" : " ,");
      }
      fprintf(stdout, "]}");
    }
    fprintf(stdout, "]}\n");
    fflush(stdout);
  } else if (show) {
    fprintf(stdout, "%scompaudit-cache: insecure %d secure %d\n",
            zp_icon("🔐 "), insecure, secure);
    if (cache_entries.size) {
      for (size_t i = 0; i < cache_entries.size; i++) {
        if (cache_entries.items[i].verdict) {
          fprintf(stdout, "  ! %s%s\n", cache_entries.items[i].path,
                  cache_entries.items[i].parent_insecure ? " (ancestor)" : "");
        }
      }
    }
    fflush(stdout);
  }
  zp_cc_vec_free(&cache_entries);
  zsfree(cache_path);
  return ret;
}
// NOLINTEND(readability-function-cognitive-complexity,bugprone-easily-swappable-parameters)

// NOLINTEND(misc-include-cleaner)
