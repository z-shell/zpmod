/* SPDX-License-Identifier: MIT */
/**
 * @file readarray.c
 * @brief readarray builtin: read records from stdin or fd into an array.
 */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_emoji.h"

static void readarray_usage(void) {
  fprintf(stdout,
          "%sUsage:%s readarray [-d delim] [-n count] [-O origin] [-s count] "
          "[-t] [-u fd] [-C callback] [-c quantum] array\n"
          "Read records from standard input (or -u fd) into the named array.\n",
          zp_icon("📥 "), "");
  fflush(stdout);
}

/** readarray builtin entrypoint */
int bin_readarray(char *nam, char **argv, UNUSED(Options ops),
                  UNUSED(int func)) {
  int srcfd = 0;
  char *callback = NULL;
  char *oarr_name = NULL;
  FILE *stream = NULL;
  if (OPT_ISSET(ops, 'u')) {
    srcfd = OPT_ARG(ops, 'u') ? atoi(OPT_ARG(ops, 'u')) : 0;
  }
  if (OPT_ISSET(ops, 'C')) {
    callback = OPT_ARG(ops, 'C') ? ztrdup(OPT_ARG(ops, 'C')) : NULL;
  }
  if (!*argv) {
    zwarnnam(nam, "%d: Name of the output array is required, aborting",
             __LINE__);
    if (callback) {
      zsfree(callback);
    }
    return 1;
  }
  oarr_name = ztrdup(*argv);
  ++argv;
  if (*argv) {
    zwarnnam(nam,
             "%d: Extra arguments detected, only one argument is needed, see "
             "-h, aborting",
             __LINE__);
    if (callback) {
      zsfree(callback);
    }
    if (oarr_name) {
      zsfree(oarr_name);
    }
    return 1;
  }
  if (!OPT_ISSET(ops, 'O')) {
    unsetparam(oarr_name);
    char **emptyarr = (char **)zalloc(sizeof(char *));
    emptyarr[0] = NULL;
    setaparam(oarr_name, emptyarr);
  }
  stream = fdopen(srcfd, "r");
  if (!stream) {
    zwarnnam(nam, "line %d: couldn't open descriptor %d: %e", __LINE__, srcfd,
             errno);
    if (callback) {
      zsfree(callback);
    }
    if (oarr_name) {
      zsfree(oarr_name);
    }
    return 1;
  }
#ifdef HAVE_GETLINE
  int delim = '\n';
  int to_copy = 0;
  int start_at = 1;
  int skip_first = 0;
  int remdel = 0;
  int quantum = 5000;
  if (OPT_ISSET(ops, 'd')) {
    delim = OPT_ARG(ops, 'd') ? OPT_ARG(ops, 'd')[0] : '\n';
  }
  if (OPT_ISSET(ops, 'n')) {
    to_copy = OPT_ARG(ops, 'n') ? atoi(OPT_ARG(ops, 'n')) : 0;
  }
  if (OPT_ISSET(ops, 'O')) {
    start_at = OPT_ARG(ops, 'O') ? atoi(OPT_ARG(ops, 'O')) : 1;
  }
  if (OPT_ISSET(ops, 's')) {
    skip_first = OPT_ARG(ops, 's') ? atoi(OPT_ARG(ops, 's')) : 0;
  }
  if (OPT_ISSET(ops, 't')) {
    remdel = 1;
  }
  if (OPT_ISSET(ops, 'c')) {
    quantum = OPT_ARG(ops, 'c') ? atoi(OPT_ARG(ops, 'c')) : 5000;
  }
  char *line = NULL;
  size_t len = 0;
  ssize_t read_len;
  int index = start_at;
  if (delim == '\n') {
    while ((read_len = getline(&line, &len, stream)) != -1) {
      if (skip_first > 0) {
        skip_first--;
        continue;
      }
      if (remdel && read_len > 0 && line[read_len - 1] == '\n') {
        line[--read_len] = '\0';
      }
      if (to_copy > 0 && index - start_at >= to_copy)
        break;
      char indexed_name[256];
      snprintf(indexed_name, sizeof(indexed_name), "%s[%d]", oarr_name, index);
      setsparam(indexed_name, line);
      if (callback && (index - start_at + 1) % quantum == 0) {
        char idx_str[20];
        snprintf(idx_str, sizeof(idx_str), "%d", index);
        char *args[] = {idx_str, line, NULL};
        execstring(callback, args, 0, 0);
      }
      index++;
    }
    free(line);
  } else {
    size_t cap = 1024, sz = 0;
    char *buf = (char *)zalloc(cap);
    int c;
    if (!buf) {
      zwarnnam(nam, "%d: Out of memory", __LINE__);
      goto done;
    }
    while ((c = fgetc(stream)) != EOF) {
      if (skip_first > 0 && c == delim) {
        skip_first--;
        sz = 0;
        continue;
      }
      if (sz + 1 >= cap) {
        size_t newcap = cap * 2;
        char *nbuf = (char *)zrealloc(buf, newcap);
        if (!nbuf) {
          zfree(buf, cap);
          zwarnnam(nam, "%d: Out of memory", __LINE__);
          goto done;
        }
        buf = nbuf;
        cap = newcap;
      }
      if (c == delim) {
        if (!remdel) {
          buf[sz++] = (char)delim;
        }
        buf[sz] = '\0';
        if (to_copy > 0 && index - start_at >= to_copy)
          break;
        char indexed_name[256];
        snprintf(indexed_name, sizeof(indexed_name), "%s[%d]", oarr_name,
                 index);
        setsparam(indexed_name, buf);
        if (callback && (index - start_at + 1) % quantum == 0) {
          char idx_str[20];
          snprintf(idx_str, sizeof(idx_str), "%d", index);
          char *args[] = {idx_str, buf, NULL};
          execstring(callback, args, 0, 0);
        }
        index++;
        sz = 0;
      } else {
        buf[sz++] = (char)c;
      }
    }
    if (sz > 0 && (to_copy == 0 || index - start_at < to_copy)) {
      buf[sz] = '\0';
      char indexed_name[256];
      snprintf(indexed_name, sizeof(indexed_name), "%s[%d]", oarr_name, index);
      setsparam(indexed_name, buf);
    }
    if (buf)
      zfree(buf, cap);
  }
#else
  (void)nam;
  (void)argv; /* no-op when getline isn't available */
#endif
done:
  if (stream) {
    fclose(stream);
  }
  if (callback) {
    zsfree(callback);
  }
  if (oarr_name) {
    zsfree(oarr_name);
  }
  return 0;
}
