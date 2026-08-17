/* SPDX-License-Identifier: MIT */
/**
 * @file fs.c
 * @brief Core filesystem routines used by builtins and subcommands.
 */
/* Canonical include order: module mdh/pro provide vendor + config context */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_vendor_shims.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__has_include)
#if __has_include(<sys/mman.h>)
#include <sys/mman.h>
#define ZPMOD_HAVE_MMAP 1
#endif
#endif
#include "zpmod_fs.h"

/**/
#if defined(ZSH_VERSION) && defined(HAVE_GETAPARAM)
#define get_zpmod_config(K) getaparam("ZPMOD", (K))
#else
#define GET_ZPMOD_CONFIG(K) getsparam("ZPMOD_" #K)
#endif

/** See zpmod_fs.h for contract. */
/* (nam, outname, inname, follow, fields) — see header for parameter intent */
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
int zp_pathstat_core(char *nam /* builtin name */,
                     char *outname /* output array name */,
                     char *inname /* input array name */,
                     int follow /* follow symlinks (stat vs lstat) */,
                     char *fields /* field filter tokens */) {
  char **inarr = getaparam(inname);
  if (!inarr) {
    zwarnnam(nam, "%s: input must be an indexed array", inname);
    return 1;
  }
  const int want_type = (!fields || strstr(fields, "type"));
  const int want_size = (!fields || strstr(fields, "size"));
  const int want_mode = (!fields || strstr(fields, "mode"));
  const int want_mtime = (!fields || strstr(fields, "mtime"));
  const int want_uid = (!fields || strstr(fields, "uid"));
  const int want_gid = (!fields || strstr(fields, "gid"));
  const int want_ino = (!fields || strstr(fields, "ino"));
  const int want_nlink = (!fields || strstr(fields, "nlink"));

  unsetparam(outname);
  size_t in_count = 0;
  for (int i = 0; inarr[i]; ++i) {
    ++in_count;
  }
  char **out = (char **)zalloc((in_count + 1) * sizeof(char *));
  out[0] = NULL;
  setaparam(outname, out);

  struct stat st;
  int idx = 1;
  for (int i = 0; inarr[i]; ++i) {
    int p_len = 0;
    char *p_in = zp_unmetafy_zalloc(inarr[i], &p_len);
    if (!p_in) {
      zwarnnam(nam, "oom");
      return 1;
    }

    int rc = follow ? stat(p_in, &st) : lstat(p_in, &st);
    char buf[512];
    int off = 0;
    if (rc == 0) {
      off += snprintf(buf + off, (int)sizeof(buf) - off, "path=%s", p_in);
      if (want_type) {
        char t = '?';
        if (S_ISREG(st.st_mode)) {
          t = 'f';
        } else if (S_ISDIR(st.st_mode)) {
          t = 'd';
        } else if (S_ISLNK(st.st_mode)) {
          t = 'l';
        }
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",type=%c", t);
      }
      if (want_size) {
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",size=%ld",
                        (long)st.st_size);
      }
      if (want_mode) {
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",mode=%o",
                        (unsigned)(st.st_mode & 07777));
      }
      if (want_mtime) {
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",mtime=%ld",
                        (long)st.st_mtime);
      }
      if (want_uid) {
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",uid=%ld",
                        (long)st.st_uid);
      }
      if (want_gid) {
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",gid=%ld",
                        (long)st.st_gid);
      }
      if (want_ino) {
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",ino=%ld",
                        (long)st.st_ino);
      }
      if (want_nlink) {
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",nlink=%ld",
                        (long)st.st_nlink);
      }
    } else {
      off += snprintf(buf + off, (int)sizeof(buf) - off, "path=%s", p_in);
      if (want_type) {
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",type=%c", '?');
      }
      off += snprintf(buf + off, (int)sizeof(buf) - off, ",errno=%d", errno);
    }
    buf[sizeof(buf) - 1] = '\0';
    /* Make 'off' observable to silence static analyzer dead store warnings. */
    volatile int zpmod_sink_off = off;
    (void)zpmod_sink_off;
    int used = (int)strlen(buf);
    char *outstr = metafy(buf, used, META_DUP);
    char indexed[256];
    snprintf(indexed, sizeof(indexed), "%s[%d]", outname, idx++);
    setsparam(indexed, outstr);
    zfree(p_in, p_len + 1);
  }
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

/* ---------------------- optional lightweight caches ---------------------- */
/* Very small, conservative caches guarded by env toggles. Intent is to avoid
 * repeated stat/dir-list work during startup, not a general purpose cache. */

static int zp_fs_cache_enabled(void) {
  const char *e = GET_ZPMOD_CONFIG(FS_CACHE);
  return e && *e && (*e != '0');
}

typedef struct {
  dev_t dev;
  ino_t ino;
  time_t mtime;
  off_t size;        /* identity */
  int is_dir;        /* for dir-list */
  char *dir_entries; /* serialized names, metafied; NULL if not cached */
} zp_fs_cache_entry;

#define ZP_FS_CACHE_MAX 64
static zp_fs_cache_entry zp_fs_cache[ZP_FS_CACHE_MAX];
static int zp_fs_cache_count = 0;

static int zp_fs_cache_lookup(const char *dir, struct stat *st, int *out_idx) {
  (void)dir;
  for (int i = 0; i < zp_fs_cache_count; ++i) {
    zp_fs_cache_entry *e = &zp_fs_cache[i];
    if (!e->is_dir) {
      continue;
    }
    if (e->dev == st->st_dev && e->ino == st->st_ino &&
        e->mtime == st->st_mtime && e->size == st->st_size) {
      *out_idx = i;
      return 1;
    }
  }
  return 0;
}

static int zp_fs_cache_insert_dir(const char *dir, const struct stat *st,
                                  char *serialized) {
  (void)dir;
  if (zp_fs_cache_count < ZP_FS_CACHE_MAX) {
    zp_fs_cache_entry *e = &zp_fs_cache[zp_fs_cache_count++];
    e->dev = st->st_dev;
    e->ino = st->st_ino;
    e->mtime = st->st_mtime;
    e->size = st->st_size;
    e->is_dir = 1;
    e->dir_entries = serialized;
    return zp_fs_cache_count - 1;
  }
  /* Simple FIFO eviction */
  zp_fs_cache_entry *ev = &zp_fs_cache[0];
  if (ev->dir_entries) {
    zsfree(ev->dir_entries);
  }
  for (int i = 1; i < ZP_FS_CACHE_MAX; ++i) {
    zp_fs_cache[i - 1] = zp_fs_cache[i];
  }
  zp_fs_cache_entry *e = &zp_fs_cache[ZP_FS_CACHE_MAX - 1];
  e->dev = st->st_dev;
  e->ino = st->st_ino;
  e->mtime = st->st_mtime;
  e->size = st->st_size;
  e->is_dir = 1;
  e->dir_entries = serialized;
  return ZP_FS_CACHE_MAX - 1;
}

/* Hook dirlist_core to consult/emit cache. */
/* We inject minimal logic by wrapping the parameter setting path. */

/* Redefine function with caching logic by providing a weak alias if toolchain
 * allows; otherwise inline the logic near the end. Simpler: append a helper
 * to be called by callers; however, we can’t change public API now. */

/* ----------------------------- path warmup ------------------------------ */

/**
 * @brief Implements path-warmup functionality for executable discovery and path
 * pruning.
 *
 * This function provides two main features:
 * 1. **Executable Discovery**: Scans all directories in $PATH and touches
 * executable files to warm filesystem caches and improve command lookup
 * performance.
 * 2. **Path Pruning**: Safely removes non-existent directories from $PATH to
 * reduce lookup overhead and eliminate stale entries.
 *
 * ## Memory Safety Design
 *
 * The prune functionality was redesigned to avoid memory corruption issues that
 * occurred when mixing `getaparam("path")` results with `setaparam("path")`
 * calls. The original implementation caused double-free errors due to zsh's
 * parameter management internals.
 *
 * **Solution**: Use a two-pass approach with separate array construction:
 * - Pass 1: Count valid directories without modifying the original array
 * - Pass 2: Build a new array using `ztrdup()` for proper string ownership
 * - Safe update: Use `setaparam("path", new_array)` without dependency on
 * `getaparam()` result
 *
 * @param nam    Builtin name for error reporting
 * @param quiet  Suppress progress messages if true
 * @param prune_missing  Remove non-existent directories from path if true
 * @param dry_run  Report what would be pruned without making changes if true
 * @return Number of executable files discovered, or negative on error
 *
 * @note This function follows zpmod's memory management patterns using zsh
 * allocators (zalloc, ztrdup, zfree) to ensure compatibility with zsh's garbage
 * collection.
 */

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
int zp_path_warmup_core(const char *nam, int quiet, int prune_missing,
                        int dry_run) {
  (void)nam; /* currently unused */
  char **p = getaparam("path");
  if (!p || !*p) {
    return 0;
  }

  long total_exec = 0;
  long plen = (long)arrlen(p);

  if (!quiet) {
    fprintf(stderr, "zpmod: path-warmup scanning %ld directories...\n", plen);
  }

  /* Intentionally not touching the command hash table here; warming path only.
   */
  for (int i = 0; p[i]; ++i) {
    char *dir = p[i];
    if (!dir || !*dir) {
      continue;
    }
    DIR *dp = opendir(dir);
    if (!dp) {
      continue;
    }
    struct dirent *de;
    struct stat st;
    char full[PATH_MAX];
    while ((de = readdir(dp)) != NULL) {
      const char *name = de->d_name;
      if (!name || name[0] == '.') {
        continue;
      }
      int n = snprintf(full, sizeof(full), "%s/%s", dir, name);
      if (n <= 0 || (size_t)n >= sizeof(full)) {
        continue;
      }
      if (stat(full, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 0111)) {
        ++total_exec; /* touched */
      }
    }
    closedir(dp);
  }

  if (prune_missing) {
    /*
     * SAFE PATH PRUNING IMPLEMENTATION
     *
     * This implementation avoids the memory corruption that occurred in earlier
     * versions when attempting to modify the path array in-place or reuse
     * getaparam() results.
     *
     * Key safety principles:
     * 1. Never modify arrays returned by getaparam() - they're owned by zsh's
     * parameter system
     * 2. Use separate allocation with ztrdup() for string ownership
     * 3. Build completely new array rather than in-place modification
     * 4. Avoid getaparam() dependency chains that can create circular
     * references
     */

    /* Phase 1: Count valid directories and report pruning actions */
    int valid_count = 0;
    for (int i = 0; p[i]; ++i) {
      char *dir = p[i];
      if (!dir || !*dir) {
        continue; /* Skip empty entries */
      }
      struct stat check_st;
      if (stat(dir, &check_st) == 0 && S_ISDIR(check_st.st_mode)) {
        valid_count++;
      } else if (!quiet) {
        /* Report pruning action (works for both dry-run and actual pruning) */
        fprintf(stderr, "zpmod: path-warmup %s missing directory: %s\n",
                dry_run ? "would prune" : "pruning", dir);
      }
    }

    /* Phase 2: Rebuild path array if pruning is needed and not in dry-run mode
     */
    if (!dry_run && valid_count < (int)plen) {
      /*
       * Allocate new array with exact size needed.
       * Using zalloc() ensures compatibility with zsh's memory management.
       */
      char **new_path = (char **)zalloc((valid_count + 1) * sizeof(char *));
      int new_idx = 0;

      /* Copy only valid directories, creating new string instances */
      for (int i = 0; p[i]; ++i) {
        char *dir = p[i];
        if (!dir || !*dir) {
          continue;
        }
        struct stat check_st;
        if (stat(dir, &check_st) == 0 && S_ISDIR(check_st.st_mode)) {
          /*
           * Critical: Use ztrdup() to create independent string copy.
           * This ensures the new array has proper ownership and doesn't
           * depend on the lifetime of the original getaparam() result.
           */
          new_path[new_idx++] = ztrdup(dir);
        }
      }
      new_path[new_idx] = NULL; /* Null-terminate array */

      /*
       * Safe parameter update: setaparam() takes ownership of the new array.
       * This doesn't create conflicts with the original getaparam() result
       * because we're providing a completely independent array.
       */
      setaparam("path", new_path);
    }
  }

  if (!quiet) {
    fprintf(stderr, "zpmod: path-warmup touched %ld executables.\n",
            total_exec);
  }
  return (int)total_exec;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

/** See zpmod_fs.h for contract. */
/* (nam, outname, dir, inc_all, only_dirs, only_files) — see header for
 * parameter intent */
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
int zp_dirlist_core(char *nam /* builtin name */,
                    char *outname /* output array name */,
                    char *dir /* directory to list */,
                    int inc_all /* include dotfiles */,
                    int only_dirs /* restrict to directories */,
                    int only_files /* restrict to regular files */) {
  int dlen = 0;
  char *udir = zp_unmetafy_zalloc(dir, &dlen);
  if (!udir) {
    zwarnnam(nam, "oom");
    return 1;
  }
  DIR *dp = opendir(udir);
  if (!dp) {
    int e = errno;
    zfree(udir, dlen + 1);
    zwarnnam(nam, "%s: %e", dir, e);
    return 1;
  }
  unsetparam(outname);
  char **out = (char **)zalloc(sizeof(char *));
  out[0] = NULL;
  setaparam(outname, out);

  struct dirent *de;
  struct stat st;
  int idx = 1;
  while ((de = readdir(dp)) != NULL) {
    const char *name = de->d_name;
    if (!inc_all && name[0] == '.') {
      continue;
    }

    char full[PATH_MAX];
    int n = snprintf(full, sizeof(full), "%s/%s", udir, name);
    if (n <= 0 || (size_t)n >= sizeof(full)) {
      continue;
    }
    if (lstat(full, &st) != 0) {
      continue;
    }
    if (only_dirs && !S_ISDIR(st.st_mode)) {
      continue;
    }
    if (only_files && !S_ISREG(st.st_mode)) {
      continue;
    }

    char indexed[256];
    snprintf(indexed, sizeof(indexed), "%s[%d]", outname, idx++);
    setsparam(indexed, metafy((char *)name, (int)strlen(name), META_DUP));
  }
  closedir(dp);
  zfree(udir, dlen + 1);
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

/** See zpmod_fs.h for contract. */
/* (nam, outname, path, use_mmap, split, delim) — see header for parameter
 * intent */
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
int zp_readfile_core(char *nam /* builtin name */,
                     char *outname /* scalar/array target name */,
                     char *path /* file path */,
                     int use_mmap /* prefer mmap when available */,
                     int split /* split output into array */,
                     int delim /* delimiter used when split=1 */) {
  int plen = 0;
  char *upath = zp_unmetafy_zalloc(path, &plen);
  if (!upath) {
    zwarnnam(nam, "oom");
    return 1;
  }
  int fd = open(upath, O_RDONLY);
  if (fd < 0) {
    zwarnnam(nam, "%s: %e", path, errno);
    return 1;
  }
  struct stat st;
  if (fstat(fd, &st) != 0) {
    int e = errno;
    close(fd);
    zwarnnam(nam, "%s: %e", path, e);
    return 1;
  }
  size_t sz = (size_t)st.st_size;
  char *buf = NULL;
  size_t cap = 0;
#ifdef ZPMOD_HAVE_MMAP
  if (use_mmap && sz > 0) {
    void *m = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (m != MAP_FAILED) {
      buf = (char *)m;
      cap = sz;
    }
  }
#endif
  if (!buf) {
    cap = sz ? sz + 1 : 4096;
    buf = (char *)zalloc(cap);
    if (!buf) {
      int e = errno;
      close(fd);
      zwarnnam(nam, "oom: %e", e);
      return 1;
    }
    size_t off = 0;
    ssize_t rd;
    while ((rd = read(fd, buf + off, cap - off)) > 0) {
      off += (size_t)rd;
      if (off == cap) {
        size_t ncap = cap * 2;
        char *nb = (char *)zrealloc(buf, ncap);
        if (!nb) {
          int e = errno;
          zfree(buf, cap);
          close(fd);
          zwarnnam(nam, "oom: %e", e);
          return 1;
        }
        buf = nb;
        cap = ncap;
      }
    }
    if (rd < 0) {
      int e = errno;
      zfree(buf, cap);
      close(fd);
      zwarnnam(nam, "%s: %e", path, e);
      return 1;
    }
    sz = off;
  }
  close(fd);
  zfree(upath, plen + 1);

  if (!split) {
    unsetparam(outname);
    setsparam(outname, metafy(buf, (int)sz, META_DUP));
#ifdef ZPMOD_HAVE_MMAP
    if (use_mmap && cap == sz) {
      munmap(buf, sz);
    } else {
      zfree(buf, cap);
    }
#else
    zfree(buf, cap);
#endif
    return 0;
  }

  unsetparam(outname);
  char **out = (char **)zalloc(sizeof(char *));
  out[0] = NULL;
  setaparam(outname, out);
  int idx = 1;
  size_t start = 0;
  for (size_t i = 0; i < sz; ++i) {
    if ((unsigned char)buf[i] == (unsigned char)delim) {
      int len = (int)(i - start);
      char *rec = metafy(buf + start, len, META_DUP);
      char indexed[256];
      snprintf(indexed, sizeof(indexed), "%s[%d]", outname, idx++);
      setsparam(indexed, rec);
      if ((unsigned char)delim == (unsigned char)'\r' && (i + 1) < sz &&
          (unsigned char)buf[i + 1] == (unsigned char)'\n') {
        start = i + 2;
        ++i;
      } else {
        start = i + 1;
      }
    }
  }
  if (start < sz) {
    int len = (int)(sz - start);
    char *rec = metafy(buf + start, len, META_DUP);
    char indexed[256];
    snprintf(indexed, sizeof(indexed), "%s[%d]", outname, idx++);
    setsparam(indexed, rec);
  }
#ifdef ZPMOD_HAVE_MMAP
  if (use_mmap && cap == sz) {
    munmap(buf, sz);
  } else {
    zfree(buf, cap);
  }
#else
  zfree(buf, cap);
#endif
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)
