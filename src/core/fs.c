/* SPDX-License-Identifier: MIT */
/**
 * @file fs.c
 * @brief Core filesystem routines used by builtins and subcommands.
 */
#include "zpmod.mdh"
#include "zpmod.pro"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(__has_include)
#if __has_include(<sys/mman.h>)
#include <sys/mman.h>
#define ZPMOD_HAVE_MMAP 1
#endif
#endif
#include "zpmod_fs.h"
#include "zpmod_utils.h"

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
    volatile int _zpmod_sink_off = off;
    (void)_zpmod_sink_off;
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
