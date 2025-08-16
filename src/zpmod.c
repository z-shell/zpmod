/* -*- Mode: C; c-default-style: "linux"; c-basic-offset: 4; indent-tabs-mode:
 * nil -*- vim:sw=4:sts=4:et
 */
/**
 * \file src/zpmod.c
 * \brief zpmod zsh module implementation.
 *
 * This file contains the implementation of the zpmod zsh module, including
 * builtin registrations, feature tables, and compatibility helpers. It is
 * intended to be parsed by Doxygen.
 */
// NOLINTBEGIN(readability-identifier-length,
// bugprone-assignment-in-if-condition, bugprone-narrowing-conversions,
// bugprone-implicit-widening-of-multiplication-result,
// bugprone-signed-char-misuse, clang-analyzer-core.NullDereference)
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_version.h"

/* Optional terminal/locale detection for emoji support */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(__has_include)
#if __has_include(<sys/mman.h>)
#include <sys/mman.h>
#define ZPMOD_HAVE_MMAP 1
#endif
#endif
#if defined(__has_include)
#if __has_include(<langinfo.h>)
#include <langinfo.h>
#define ZPMOD_HAVE_LANGINFO 1
#endif
#endif

/* Forward declarations for internal helpers used before definition */
mod_export enum source_return custom_source(char *s);
Eprog custom_try_source_file(char *file);
static Eprog custom_check_dump_file(char *file, struct stat *sbuf, char *name,
                                    int *ksh, int test_only);
static Wordcode custom_load_dump_header(char *nam, char *name, int err);
static void readarray_usage(void);
static int zp_append_report(const char *nam, const char *target, int target_len,
                            const char *body, int body_len);
static void zp_free_sevent_node(HashNode hn);
static int zp_has_option(char **argv, char opt);
static int zp_icons_enabled(void);
static const char *zp_icon(const char *s);

/* Determine if we should emit icons/emojis: TTY + UTF-8 locale + optional env
 * override. */
static int zp_icons_enabled(void) {
  static int cached = -1; /* -1 = unknown, 0/1 = computed */
  if (cached != -1)
    return cached;

  const char *env = getsparam("ZPMOD_ICONS");
  if (env) {
    if (!strcmp(env, "0") || !strcmp(env, "false") || !strcmp(env, "off")) {
      cached = 0;
      return cached;
    }
    if (!strcmp(env, "1") || !strcmp(env, "true") || !strcmp(env, "on")) {
      cached = 1;
      return cached;
    }
  }
  if (!isatty(STDOUT_FILENO)) {
    cached = 0;
    return cached;
  }
  /* Check locale looks like UTF-8 */
  setlocale(LC_ALL, "");
#ifdef ZPMOD_HAVE_LANGINFO
  const char *cs = nl_langinfo(CODESET);
  if (cs && (strstr(cs, "UTF-8") || strstr(cs, "utf8") || strstr(cs, "UTF8"))) {
    cached = 1;
    return cached;
  }
#else
  /* Fallback: check LC_ALL/LANG env vars */
  const char *lc = getenv("LC_ALL");
  if (!lc)
    lc = getenv("LANG");
  if (lc && (strstr(lc, "UTF-8") || strstr(lc, "utf8") || strstr(lc, "UTF8"))) {
    cached = 1;
    return cached;
  }
#endif
  cached = 0;
  return cached;
}

/* Return icon string if enabled, else empty string. */
static const char *zp_icon(const char *s) {
  return zp_icons_enabled() ? s : "";
}

/* =============================
 * Fast filesystem helpers
 * ============================= */

/* Shared helpers for fast FS builtins */
static int zp_pathstat_core(char *nam, char *outname, char *inname, int follow,
                            char *fields);
static int zp_dirlist_core(char *nam, char *outname, char *dir, int inc_all,
                           int only_dirs, int only_files);
static int zp_readfile_core(char *nam, char *outname, char *path, int use_mmap,
                            int split, int delim);

/* zppathstat: batch stat/lstat on an input array of paths.
 * Usage: zppathstat [-L] [-f fields] out_array in_array
 * Fields default: type,size,mode,mtime. Output format per element:
 *   path=...,type=f|d|l|?,size=...,mode=octal,mtime=epoch
 * On error for an item, includes errno=NUM and type=?
 */
static int bin_zppathstat(char *nam, char **argv, UNUSED(Options ops),
                          UNUSED(int func)) {
  int follow = OPT_ISSET(ops, 'L');
  char *fields = NULL;
  if (OPT_ISSET(ops, 'f'))
    fields = OPT_ARG(ops, 'f');
  if (!argv || !argv[0] || !argv[1]) {
    zwarnnam(nam, "usage: %s [-L] [-f fields] out_array in_array", nam);
    return 1;
  }
  return zp_pathstat_core(nam, argv[0], argv[1], follow, fields);
}

/* zpdirlist: list entries in dir (no recursion).
 * Usage: zpdirlist [-a] [-d] [-f] out_array dir
 * -a include dotfiles
 * -d only directories
 * -f only regular files
 */
static int bin_zpdirlist(char *nam, char **argv, UNUSED(Options ops),
                         UNUSED(int func)) {
  int inc_all = OPT_ISSET(ops, 'a');
  int only_dirs = OPT_ISSET(ops, 'd');
  int only_files = OPT_ISSET(ops, 'f');
  if (!argv || !argv[0] || !argv[1]) {
    zwarnnam(nam, "usage: %s [-a] [-d] [-f] out_array dir", nam);
    return 1;
  }
  return zp_dirlist_core(nam, argv[0], argv[1], inc_all, only_dirs, only_files);
}

/* zpreadfile: read entire file into scalar or array split by delim */
static int bin_zpreadfile(char *nam, char **argv, UNUSED(Options ops),
                          UNUSED(int func)) {
  int use_mmap = OPT_ISSET(ops, 'm');
  int delim = '\n';
  int split = 0;
  if (OPT_ISSET(ops, '0')) {
    split = 1;
    delim = '\0';
  }
  if (OPT_ISSET(ops, 'd')) {
    char *a = OPT_ARG(ops, 'd');
    if (a && *a) {
      split = 1;
      if (a[0] == '\\') {
        switch (a[1]) {
        case 'n':
          delim = '\n';
          break;
        case 't':
          delim = '\t';
          break;
        case '0':
          delim = '\0';
          break;
        case 'r':
          delim = '\r';
          break;
        default:
          delim = (unsigned char)a[1];
          break;
        }
      } else {
        delim = (unsigned char)a[0];
      }
    }
  }
  if (!argv || !argv[0] || !argv[1]) {
    zwarnnam(nam, "usage: %s [-m] [-d delim|-0] out file", nam);
    return 1;
  }
  return zp_readfile_core(nam, argv[0], argv[1], use_mmap, split, delim);
}

/* ===== Shared helper implementations ===== */
static int zp_pathstat_core(char *nam, char *outname, char *inname, int follow,
                            char *fields) {
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
  char **out = (char **)zalloc(sizeof(char *));
  out[0] = NULL;
  setaparam(outname, out);

  struct stat st;
  int idx = 1;
  for (int i = 0; inarr[i]; ++i) {
    /* Input from zsh is metafied; create an unmetafied copy for syscalls */
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
        if (S_ISREG(st.st_mode))
          t = 'f';
        else if (S_ISDIR(st.st_mode))
          t = 'd';
        else if (S_ISLNK(st.st_mode))
          t = 'l';
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",type=%c", t);
      }
      if (want_size)
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",size=%ld",
                        (long)st.st_size);
      if (want_mode)
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",mode=%o",
                        (unsigned)(st.st_mode & 07777));
      if (want_mtime)
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",mtime=%ld",
                        (long)st.st_mtime);
      if (want_uid)
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",uid=%ld",
                        (long)st.st_uid);
      if (want_gid)
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",gid=%ld",
                        (long)st.st_gid);
      if (want_ino)
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",ino=%ld",
                        (long)st.st_ino);
      if (want_nlink)
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",nlink=%ld",
                        (long)st.st_nlink);
    } else {
      off += snprintf(buf + off, (int)sizeof(buf) - off, "path=%s", p_in);
      if (want_type)
        off += snprintf(buf + off, (int)sizeof(buf) - off, ",type=%c", '?');
      off += snprintf(buf + off, (int)sizeof(buf) - off, ",errno=%d", errno);
    }
    buf[sizeof(buf) - 1] = '\0';
    /* Re-metafy before storing into a shell parameter. Use the actual
     * string length to avoid reading past the buffer if we truncated. */
    int used = (int)strlen(buf);
    char *outstr = metafy(buf, used, META_DUP);
    char indexed[256];
    snprintf(indexed, sizeof(indexed), "%s[%d]", outname, idx++);
    setsparam(indexed, outstr);
    zfree(p_in, p_len + 1);
  }
  return 0;
}

static int zp_dirlist_core(char *nam, char *outname, char *dir, int inc_all,
                           int only_dirs, int only_files) {
  /* Unmetafy dir for syscalls */
  int dlen = 0;
  char *udir = zp_unmetafy_zalloc(dir, &dlen);
  if (!udir) {
    zwarnnam(nam, "oom");
    return 1;
  }
  DIR *dp = opendir(udir);
  if (!dp) {
    zwarnnam(nam, "%s: %e", dir, errno);
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
    if (!inc_all && name[0] == '.')
      continue;
    if (only_dirs || only_files) {
      char full[PATH_MAX];
      int n = snprintf(full, sizeof(full), "%s/%s", udir, name);
      if (n <= 0 || (size_t)n >= sizeof(full))
        continue;
      if (lstat(full, &st) != 0)
        continue;
      if (only_dirs && !S_ISDIR(st.st_mode))
        continue;
      if (only_files && !S_ISREG(st.st_mode))
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

static int zp_readfile_core(char *nam, char *outname, char *path, int use_mmap,
                            int split, int delim) {
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
    if (use_mmap && cap == sz)
      munmap(buf, sz);
    else
      zfree(buf, cap);
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
      start = i + 1;
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
  if (use_mmap && cap == sz)
    munmap(buf, sz);
  else
    zfree(buf, cap);
#else
  zfree(buf, cap);
#endif
  return 0;
}

/* Source/bin_dot related data structures */
/**
 * Runtime tracking for files loaded via the overridden '.' and 'source'
 * builtins.
 *
 * The module intercepts sourcing to measure load duration and records events in
 * a private hashtable (zp_source_events). These entries are later reported by
 * the `zpmod source-study` subcommand.
 */
static HandlerFunc originalDot = NULL, originalSource = NULL;
static HashTable zp_source_events = NULL;
static int zp_sevent_count = 0;

/**
 * \brief Captured metrics and identifiers for a single source() event.
 */
struct source_event {
  int id;
  long ts;
  char *dir_path;
  char *file_name;
  char *full_path;
  double duration;
  int load_error;
};

/**
 * \brief Hashtable node wrapper for source_event, compatible with zsh's API.
 */
struct zp_sevent_node {
  struct hashnode node;
  struct source_event event;
};

typedef struct zp_sevent_node *SEventNode;

/* Option support */
/**
 * Version-stable option index mapping.
 *
 * zsh's internal indices for options vary across versions. This table is
 * populated at runtime by zp_setup_options_table() so the rest of the code can
 * refer to options via stable enum values. See zp_conv_opt().
 */
static int zp_opt_for_zsh_version[256] = {0};

enum {
  OPT_INVALID__,
  ALIASESOPT__,
  ALIASFUNCDEF__,
  ALLEXPORT__,
  ALWAYSLASTPROMPT__,
  ALWAYSTOEND__,
  APPENDHISTORY__,
  AUTOCD__,
  AUTOCONTINUE__,
  AUTOLIST__,
  AUTOMENU__,
  AUTONAMEDIRS__,
  AUTOPARAMKEYS__,
  AUTOPARAMSLASH__,
  AUTOPUSHD__,
  AUTOREMOVESLASH__,
  AUTORESUME__,
  BADPATTERN__,
  BANGHIST__,
  BAREGLOBQUAL__,
  BASHAUTOLIST__,
  BASHREMATCH__,
  BEEP__,
  BGNICE__,
  BRACECCL__,
  BSDECHO__,
  CASEGLOB__,
  CASEMATCH__,
  CBASES__,
  CDABLEVARS__,
  CHASEDOTS__,
  CHASELINKS__,
  CHECKJOBS__,
  CHECKRUNNINGJOBS__,
  CLOBBER__,
  APPENDCREATE__,
  COMBININGCHARS__,
  COMPLETEALIASES__,
  COMPLETEINWORD__,
  CORRECT__,
  CORRECTALL__,
  CONTINUEONERROR__,
  CPRECEDENCES__,
  CSHJUNKIEHISTORY__,
  CSHJUNKIELOOPS__,
  CSHJUNKIEQUOTES__,
  CSHNULLCMD__,
  CSHNULLGLOB__,
  DEBUGBEFORECMD__,
  EMACSMODE__,
  EQUALS__,
  ERREXIT__,
  ERRRETURN__,
  EXECOPT__,
  EXTENDEDGLOB__,
  EXTENDEDHISTORY__,
  EVALLINENO__,
  FLOWCONTROL__,
  FORCEFLOAT__,
  FUNCTIONARGZERO__,
  GLOBOPT__,
  GLOBALEXPORT__,
  GLOBALRCS__,
  GLOBASSIGN__,
  GLOBCOMPLETE__,
  GLOBDOTS__,
  GLOBSTARSHORT__,
  GLOBSUBST__,
  HASHCMDS__,
  HASHDIRS__,
  HASHEXECUTABLESONLY__,
  HASHLISTALL__,
  HISTALLOWCLOBBER__,
  HISTBEEP__,
  HISTEXPIREDUPSFIRST__,
  HISTFCNTLLOCK__,
  HISTFINDNODUPS__,
  HISTIGNOREALLDUPS__,
  HISTIGNOREDUPS__,
  HISTIGNORESPACE__,
  HISTLEXWORDS__,
  HISTNOFUNCTIONS__,
  HISTNOSTORE__,
  HISTREDUCEBLANKS__,
  HISTSAVEBYCOPY__,
  HISTSAVENODUPS__,
  HISTSUBSTPATTERN__,
  HISTVERIFY__,
  HUP__,
  IGNOREBRACES__,
  IGNORECLOSEBRACES__,
  IGNOREEOF__,
  INCAPPENDHISTORY__,
  INCAPPENDHISTORYTIME__,
  INTERACTIVE__,
  INTERACTIVECOMMENTS__,
  KSHARRAYS__,
  KSHAUTOLOAD__,
  KSHGLOB__,
  KSHOPTIONPRINT__,
  KSHTYPESET__,
  KSHZEROSUBSCRIPT__,
  LISTAMBIGUOUS__,
  LISTBEEP__,
  LISTPACKED__,
  LISTROWSFIRST__,
  LISTTYPES__,
  LOCALLOOPS__,
  LOCALOPTIONS__,
  LOCALPATTERNS__,
  LOCALTRAPS__,
  LOGINSHELL__,
  LONGLISTJOBS__,
  MAGICEQUALSUBST__,
  MAILWARNING__,
  MARKDIRS__,
  MENUCOMPLETE__,
  MONITOR__,
  MULTIBYTE__,
  MULTIFUNCDEF__,
  MULTIOS__,
  NOMATCH__,
  NOTIFY__,
  NULLGLOB__,
  NUMERICGLOBSORT__,
  OCTALZEROES__,
  OVERSTRIKE__,
  PATHDIRS__,
  PATHSCRIPT__,
  PIPEFAIL__,
  POSIXALIASES__,
  POSIXARGZERO__,
  POSIXBUILTINS__,
  POSIXCD__,
  POSIXIDENTIFIERS__,
  POSIXJOBS__,
  POSIXSTRINGS__,
  POSIXTRAPS__,
  PRINTEIGHTBIT__,
  PRINTEXITVALUE__,
  PRIVILEGED__,
  PROMPTBANG__,
  PROMPTCR__,
  PROMPTPERCENT__,
  PROMPTSP__,
  PROMPTSUBST__,
  PUSHDIGNOREDUPS__,
  PUSHDMINUS__,
  PUSHDSILENT__,
  PUSHDTOHOME__,
  RCEXPANDPARAM__,
  RCQUOTES__,
  RCS__,
  RECEXACT__,
  REMATCHPCRE__,
  RESTRICTED__,
  RMSTARSILENT__,
  RMSTARWAIT__,
  SHAREHISTORY__,
  SHFILEEXPANSION__,
  SHGLOB__,
  SHINSTDIN__,
  SHNULLCMD__,
  SHOPTIONLETTERS__,
  SHORTLOOPS__,
  SHWORDSPLIT__,
  SINGLECOMMAND__,
  SINGLELINEZLE__,
  SOURCETRACE__,
  SUNKEYBOARDHACK__,
  TRANSIENTRPROMPT__,
  TRAPSASYNC__,
  TYPESETSILENT__,
  UNSET__,
  VERBOSE__,
  VIMODE__,
  WARNCREATEGLOBAL__,
  WARNNESTEDVAR__,
  XTRACE__,
  USEZLE__,
  DVORAK__,
  OPT_SIZE__
};

struct zp_option_name {
  const char *name;
  int enum_val;
};

static struct zp_option_name zp_options[] = {
    {"aliases", ALIASESOPT__},
    {"aliasfuncdef", ALIASFUNCDEF__},
    {"allexport", ALLEXPORT__},
    {"alwayslastprompt", ALWAYSLASTPROMPT__},
    {"alwaystoend", ALWAYSTOEND__},
    {"appendcreate", APPENDCREATE__},
    {"appendhistory", APPENDHISTORY__},
    {"autocd", AUTOCD__},
    {"autocontinue", AUTOCONTINUE__},
    {"autolist", AUTOLIST__},
    {"automenu", AUTOMENU__},
    {"autonamedirs", AUTONAMEDIRS__},
    {"autoparamkeys", AUTOPARAMKEYS__},
    {"autoparamslash", AUTOPARAMSLASH__},
    {"autopushd", AUTOPUSHD__},
    {"autoremoveslash", AUTOREMOVESLASH__},
    {"autoresume", AUTORESUME__},
    {"badpattern", BADPATTERN__},
    {"banghist", BANGHIST__},
    {"bareglobqual", BAREGLOBQUAL__},
    {"bashautolist", BASHAUTOLIST__},
    {"bashrematch", BASHREMATCH__},
    {"beep", BEEP__},
    {"bgnice", BGNICE__},
    {"braceccl", BRACECCL__},
    {"bsdecho", BSDECHO__},
    {"caseglob", CASEGLOB__},
    {"casematch", CASEMATCH__},
    {"cbases", CBASES__},
    {"cprecedences", CPRECEDENCES__},
    {"cdablevars", CDABLEVARS__},
    {"chasedots", CHASEDOTS__},
    {"chaselinks", CHASELINKS__},
    {"checkjobs", CHECKJOBS__},
    {"checkrunningjobs", CHECKRUNNINGJOBS__},
    {"clobber", CLOBBER__},
    {"combiningchars", COMBININGCHARS__},
    {"completealiases", COMPLETEALIASES__},
    {"completeinword", COMPLETEINWORD__},
    {"continueonerror", CONTINUEONERROR__},
    {"correct", CORRECT__},
    {"correctall", CORRECTALL__},
    {"cshjunkiehistory", CSHJUNKIEHISTORY__},
    {"cshjunkieloops", CSHJUNKIELOOPS__},
    {"cshjunkiequotes", CSHJUNKIEQUOTES__},
    {"cshnullcmd", CSHNULLCMD__},
    {"cshnullglob", CSHNULLGLOB__},
    {"debugbeforecmd", DEBUGBEFORECMD__},
    {"emacs", EMACSMODE__},
    {"equals", EQUALS__},
    {"errexit", ERREXIT__},
    {"errreturn", ERRRETURN__},
    {"exec", EXECOPT__},
    {"extendedglob", EXTENDEDGLOB__},
    {"extendedhistory", EXTENDEDHISTORY__},
    {"evallineno", EVALLINENO__},
    {"flowcontrol", FLOWCONTROL__},
    {"forcefloat", FORCEFLOAT__},
    {"functionargzero", FUNCTIONARGZERO__},
    {"glob", GLOBOPT__},
    {"globalexport", GLOBALEXPORT__},
    {"globalrcs", GLOBALRCS__},
    {"globassign", GLOBASSIGN__},
    {"globcomplete", GLOBCOMPLETE__},
    {"globdots", GLOBDOTS__},
    {"globstarshort", GLOBSTARSHORT__},
    {"globsubst", GLOBSUBST__},
    {"hashcmds", HASHCMDS__},
    {"hashdirs", HASHDIRS__},
    {"hashexecutablesonly", HASHEXECUTABLESONLY__},
    {"hashlistall", HASHLISTALL__},
    {"histallowclobber", HISTALLOWCLOBBER__},
    {"histbeep", HISTBEEP__},
    {"histexpiredupsfirst", HISTEXPIREDUPSFIRST__},
    {"histfcntllock", HISTFCNTLLOCK__},
    {"histfindnodups", HISTFINDNODUPS__},
    {"histignorealldups", HISTIGNOREALLDUPS__},
    {"histignoredups", HISTIGNOREDUPS__},
    {"histignorespace", HISTIGNORESPACE__},
    {"histlexwords", HISTLEXWORDS__},
    {"histnofunctions", HISTNOFUNCTIONS__},
    {"histnostore", HISTNOSTORE__},
    {"histsubstpattern", HISTSUBSTPATTERN__},
    {"histreduceblanks", HISTREDUCEBLANKS__},
    {"histsavebycopy", HISTSAVEBYCOPY__},
    {"histsavenodups", HISTSAVENODUPS__},
    {"histverify", HISTVERIFY__},
    {"hup", HUP__},
    {"ignorebraces", IGNOREBRACES__},
    {"ignoreclosebraces", IGNORECLOSEBRACES__},
    {"ignoreeof", IGNOREEOF__},
    {"incappendhistory", INCAPPENDHISTORY__},
    {"incappendhistorytime", INCAPPENDHISTORYTIME__},
    {"interactive", INTERACTIVE__},
    {"interactivecomments", INTERACTIVECOMMENTS__},
    {"ksharrays", KSHARRAYS__},
    {"kshautoload", KSHAUTOLOAD__},
    {"kshglob", KSHGLOB__},
    {"kshoptionprint", KSHOPTIONPRINT__},
    {"kshtypeset", KSHTYPESET__},
    {"kshzerosubscript", KSHZEROSUBSCRIPT__},
    {"listambiguous", LISTAMBIGUOUS__},
    {"listbeep", LISTBEEP__},
    {"listpacked", LISTPACKED__},
    {"listrowsfirst", LISTROWSFIRST__},
    {"listtypes", LISTTYPES__},
    {"localoptions", LOCALOPTIONS__},
    {"localloops", LOCALLOOPS__},
    {"localpatterns", LOCALPATTERNS__},
    {"localtraps", LOCALTRAPS__},
    {"login", LOGINSHELL__},
    {"longlistjobs", LONGLISTJOBS__},
    {"magicequalsubst", MAGICEQUALSUBST__},
    {"mailwarning", MAILWARNING__},
    {"markdirs", MARKDIRS__},
    {"menucomplete", MENUCOMPLETE__},
    {"monitor", MONITOR__},
    {"multibyte", MULTIBYTE__},
    {"multifuncdef", MULTIFUNCDEF__},
    {"multios", MULTIOS__},
    {"nomatch", NOMATCH__},
    {"notify", NOTIFY__},
    {"nullglob", NULLGLOB__},
    {"numericglobsort", NUMERICGLOBSORT__},
    {"octalzeroes", OCTALZEROES__},
    {"overstrike", OVERSTRIKE__},
    {"pathdirs", PATHDIRS__},
    {"pathscript", PATHSCRIPT__},
    {"pipefail", PIPEFAIL__},
    {"posixaliases", POSIXALIASES__},
    {"posixargzero", POSIXARGZERO__},
    {"posixbuiltins", POSIXBUILTINS__},
    {"posixcd", POSIXCD__},
    {"posixidentifiers", POSIXIDENTIFIERS__},
    {"posixjobs", POSIXJOBS__},
    {"posixstrings", POSIXSTRINGS__},
    {"posixtraps", POSIXTRAPS__},
    {"printeightbit", PRINTEIGHTBIT__},
    {"printexitvalue", PRINTEXITVALUE__},
    {"privileged", PRIVILEGED__},
    {"promptbang", PROMPTBANG__},
    {"promptcr", PROMPTCR__},
    {"promptpercent", PROMPTPERCENT__},
    {"promptsp", PROMPTSP__},
    {"promptsubst", PROMPTSUBST__},
    {"pushdignoredups", PUSHDIGNOREDUPS__},
    {"pushdminus", PUSHDMINUS__},
    {"pushdsilent", PUSHDSILENT__},
    {"pushdtohome", PUSHDTOHOME__},
    {"rcexpandparam", RCEXPANDPARAM__},
    {"rcquotes", RCQUOTES__},
    {"rcs", RCS__},
    {"recexact", RECEXACT__},
    {"rematchpcre", REMATCHPCRE__},
    {"restricted", RESTRICTED__},
    {"rmstarsilent", RMSTARSILENT__},
    {"rmstarwait", RMSTARWAIT__},
    {"sharehistory", SHAREHISTORY__},
    {"shfileexpansion", SHFILEEXPANSION__},
    {"shglob", SHGLOB__},
    {"shinstdin", SHINSTDIN__},
    {"shnullcmd", SHNULLCMD__},
    {"shoptionletters", SHOPTIONLETTERS__},
    {"shortloops", SHORTLOOPS__},
    {"shwordsplit", SHWORDSPLIT__},
    {"singlecommand", SINGLECOMMAND__},
    {"singlelinezle", SINGLELINEZLE__},
    {"sourcetrace", SOURCETRACE__},
    {"sunkeyboardhack", SUNKEYBOARDHACK__},
    {"transientrprompt", TRANSIENTRPROMPT__},
    {"trapsasync", TRAPSASYNC__},
    {"typesetsilent", TYPESETSILENT__},
    {"unset", UNSET__},
    {"verbose", VERBOSE__},
    {"vi", VIMODE__},
    {"warncreateglobal", WARNCREATEGLOBAL__},
    {"warnnestedvar", WARNNESTEDVAR__},
    {"xtrace", XTRACE__},
    {"zle", USEZLE__},
    {"dvorak", DVORAK__},
    /* Below follow *aliases*, i.e. not-main, alternate option names */
    /* There are 10 uncommented entries */
    /* {"braceexpand",         -IGNOREBRACES__}, */
    {"dotglob", GLOBDOTS__},
    {"hashall", HASHCMDS__},
    {"histappend", APPENDHISTORY__},
    {"histexpand", BANGHIST__},
    /* {"log",                 -HISTNOFUNCTIONS__}, */
    {"mailwarn", MAILWARNING__},
    {"onecmd", SINGLECOMMAND__},
    {"physical", CHASELINKS__},
    {"promptvars", PROMPTSUBST__},
    {"stdin", SHINSTDIN__},
    {"trackall", HASHCMDS__},
    {NULL, 0}};
/*  */

/* Copied, repeated Zsh macros, data structures, etc. */
#define FD_EXT ".zwc"
#define FD_MINMAP 4096

#define FD_PRELEN 12
#define FD_MAGIC 0x04050607
#define FD_OMAGIC 0x07060504

#define FDF_MAP 1
#define FDF_OTHER 2

typedef struct fdhead *FDHead;

struct fdhead {
  wordcode start; /* offset to function definition */
  wordcode len;   /* length of wordcode/strings */
  wordcode npats; /* number of patterns needed */
  wordcode strs;  /* offset to strings */
  wordcode hlen;  /* header length (incl. name) */
  wordcode flags; /* flags and offset to name tail */
};

#define fdheaderlen(f) (((Wordcode)(f))[FD_PRELEN])

#define fdmagic(f) (((Wordcode)(f))[0])
#define fdsetbyte(f, i, v)                                                     \
  ((((unsigned char *)(((Wordcode)(f)) + 1))[i]) = ((unsigned char)(v)))
#define fdbyte(f, i) ((wordcode)(((unsigned char *)(((Wordcode)(f)) + 1))[i]))
#define fdflags(f) fdbyte(f, 0)
#define fdsetflags(f, v) fdsetbyte(f, 0, v)
#define fdother(f) (fdbyte(f, 1) + (fdbyte(f, 2) << 8) + (fdbyte(f, 3) << 16))
#define fdsetother(f, o)                                                       \
  do {                                                                         \
    fdsetbyte(f, 1, ((o)&0xff));                                               \
    fdsetbyte(f, 2, (((o) >> 8) & 0xff));                                      \
    fdsetbyte(f, 3, (((o) >> 16) & 0xff));                                     \
  } while (0)
#define fdversion(f) ((char *)((f) + 2))

#define firstfdhead(f) ((FDHead)(((Wordcode)(f)) + FD_PRELEN))
#define nextfdhead(f) ((FDHead)(((Wordcode)(f)) + (f)->hlen))

#define fdhflags(f) (((FDHead)(f))->flags)
#define fdhtail(f) (((FDHead)(f))->flags >> 2)
#define fdhbldflags(f, t) ((f) | ((t) << 2))

#define FDHF_KSHLOAD 1
#define FDHF_ZSHLOAD 2

#define fdname(f) ((char *)(((FDHead)(f)) + 1))
/*  */

/*
 * Compatibility functions (i.e. support for multiple Zsh versions)
 */

/* STATIC FUNCTION: zp_setup_options_table */
/**/
static void zp_setup_options_table() {
  int i;
  int optno;
  for (i = 0; zp_options[i].name != NULL; ++i) {
    int e = zp_options[i].enum_val;
    if (e < 0 || e >= (int)(sizeof(zp_opt_for_zsh_version) /
                            sizeof(zp_opt_for_zsh_version[0]))) {
      continue;
    }
    optno = optlookup(zp_options[i].name);
    if (optno >= 0) {
      zp_opt_for_zsh_version[e] = optno;
    } else {
      zwarn("readarray: unknown option: %s", zp_options[i].name);
    }
  }
}
/*  */
/* STATIC FUNCTION: zp_conv_opt */
/**
 * Map a stable option enum to the current zsh's option index, preserving sign.
 *
 * \param zp_opt_num Stable option id (positive to query/set, negative for
 * unset). \return The version-specific option index with input sign applied.
 */
static int zp_conv_opt(int zp_opt_num) {
  int sign = (zp_opt_num >= 0) ? 1 : -1;
  int idx = sign * zp_opt_num; /* absolute value within mapping table */
  /* Guard against out-of-range just in case */
  if (idx < 0 || idx >= (int)(sizeof(zp_opt_for_zsh_version) /
                              sizeof(zp_opt_for_zsh_version[0]))) {
    return 0; /* unknown -> 0 (invalid) */
  }
  return sign * zp_opt_for_zsh_version[idx];
}
/*  */

/*
 * `.' and `source' overload (profiling loading times)
 */

/* bin_custom_dot */
/**
 * Replacement handler for both "." and "source" builtins, with profiling.
 *
 * Replicates the original builtin search semantics and delegates execution to
 * custom_source(). The setup_() hook wires this function to both builtins and
 * stores their original handlers so finish_() can restore them.
 *
 * \param name The invoked builtin name ("." or "source").
 * \param argv Arguments, where argv[0] is the path to source and argv[1..] are
 * $1..$n. \return Exit status following zsh source semantics.
 */
int bin_custom_dot(char *name, char **argv, UNUSED(Options ops),
                   UNUSED(int func)) {
  char **old;
  char *old0 = NULL;
  int diddot = 0;
  int dotdot = 0;
  char *s;
  char **t;
  char *enam;
  char *arg0;
  char *buf;
  struct stat st;
  enum source_return ret;

  if (!*argv) {
    return 0;
  }
  old = pparams;
  /* get arguments for the script */
  if (argv[1]) {
    pparams = zarrdup(argv + 1);
  }

  enam = arg0 = ztrdup(*argv);
  if (isset(zp_conv_opt(FUNCTIONARGZERO__))) {
    old0 = argzero;
    argzero = ztrdup(arg0);
  }
  s = unmeta(enam);
  errno = ENOENT;
  ret = SOURCE_NOT_FOUND;
  /* for source only, check in current directory first */
  if (*name != '.' && access(s, F_OK) == 0 && stat(s, &st) >= 0 &&
      !S_ISDIR(st.st_mode)) {
    diddot = 1;
    ret = custom_source(enam);
  }
  if (ret == SOURCE_NOT_FOUND) {
    /* use a path with / in it */
    for (s = arg0; *s; s++) {
      if (*s == '/') {
        if (*arg0 == '.') {
          if (arg0 + 1 == s) {
            ++diddot;
          } else if (arg0[1] == '.' && arg0 + 2 == s) {
            ++dotdot;
          }
        }
        ret = custom_source(arg0);
        break;
      }
    }
    if (!*s || (ret == SOURCE_NOT_FOUND && isset(zp_conv_opt(PATHDIRS__)) &&
                diddot < 2 && dotdot == 0)) {
      pushheap();
      /* search path for script */
      for (t = path; *t; t++) {
        if (!(*t)[0] || ((*t)[0] == '.' && !(*t)[1])) {
          if (diddot) {
            continue;
          }
          diddot = 1;
          buf = dupstring(arg0);
        } else {
          buf = zhtricat(*t, "/", arg0);
        }

        s = unmeta(buf);
        if (access(s, F_OK) == 0 && stat(s, &st) >= 0 && !S_ISDIR(st.st_mode)) {
          ret = custom_source(enam = buf);
          break;
        }
      }
      popheap();
    }
  }
  /* clean up and return */
  if (argv[1]) {
    freearray(pparams);
    pparams = old;
  }
  if (ret == SOURCE_NOT_FOUND) {
    if (isset(zp_conv_opt(POSIXBUILTINS__))) {
      /* hard error in POSIX (we'll exit later) */
      zerrnam(name, "%e: %s", errno, enam);
    } else {
      zwarnnam(name, "%e: %s", errno, enam);
    }
  }
  zsfree(arg0);
  if (old0) {
    zsfree(argzero);
    argzero = old0;
  }
  return ret == SOURCE_OK ? lastval : 128 - ret;
}
/*  */
/* custom_source */
/**
 * Core implementation of sourcing a file with state preservation and timing.
 *
 * Attempts to execute a compiled zwc via custom_try_source_file(); otherwise
 * reads and executes the script from disk. Saves and restores shell state
 * (SHIN, subsh, job, options, function stack, etc.) to emulate source.
 * Captures timing metadata and records an event for reporting.
 *
 * \param s Metafied path to the script.
 * \return SOURCE_OK on success; SOURCE_NOT_FOUND or SOURCE_ERROR otherwise.
 */
mod_export enum source_return custom_source(char *s) {
  Eprog prog;
  int tempfd = -1;
  int fd;
  int cj;
  zlong oldlineno;
  int oldshst;
  int osubsh;
  int oloops;
  char *old_scriptname = scriptname;
  char *us;
  char *old_scriptfilename = scriptfilename;
  unsigned char *ocs;
  int ocsp;
  int otrap_return = trap_return;
  int otrap_state = trap_state;
  struct funcstack fstack;
  enum source_return ret = SOURCE_OK;

  /* ZP-CODE */
  SEventNode zp_node;
  struct timeval zp_tv;
  struct timezone zp_dummy_tz;
  double zp_prev_tv;
  zp_tv.tv_sec = zp_tv.tv_usec = 0;
  gettimeofday(&zp_tv, &zp_dummy_tz);
  zp_prev_tv =
      ((((double)zp_tv.tv_sec) * 1000.0) + (((double)zp_tv.tv_usec) / 1000.0));

  if (!s || (!(prog = custom_try_source_file((us = unmeta(s)))) &&
             (tempfd = movefd(open(us, O_RDONLY | O_NOCTTY))) == -1)) {
    return SOURCE_NOT_FOUND;
  }

  /* save the current shell state */
  fd = SHIN;          /* store the shell input fd                  */
  osubsh = subsh;     /* store whether we are in a subshell        */
  cj = thisjob;       /* store our current job number              */
  oldlineno = lineno; /* store our current lineno                  */
  oloops = loops;     /* stored the # of nested loops we are in    */
  oldshst =
      opts[zp_conv_opt(SHINSTDIN__)]; /* store current value of this option */
  ocs = cmdstack;
  ocsp = cmdsp;
  cmdstack = (unsigned char *)zalloc(CMDSTACKSZ);
  cmdsp = 0;

  if (!prog) {
    SHIN = tempfd;
    shinbufsave();
  }
  subsh = 0;
  lineno = 1;
  loops = 0;
  dosetopt(zp_conv_opt(SHINSTDIN__), 0, 1, opts);
  scriptname = s;
  scriptfilename = s;

  if (isset(zp_conv_opt(SOURCETRACE__))) {
    printprompt4();
    fprintf(xtrerr ? xtrerr : stderr, "<sourcetrace>\n");
  }

  /*
   * The special return behaviour of traps shouldn't
   * trigger in files sourced from traps; the return
   * is just a return from the file.
   */
  trap_state = TRAP_STATE_INACTIVE;

  sourcelevel++;

  fstack.name = scriptfilename;
  fstack.caller =
      funcstack ? funcstack->name
                : dupstring(old_scriptfilename ? old_scriptfilename : "zsh");
  fstack.flineno = 0;
  fstack.lineno = oldlineno;
  fstack.filename = scriptfilename;
  fstack.prev = funcstack;
  fstack.tp = FS_SOURCE;
  funcstack = &fstack;

  if (prog) {
    pushheap();
    errflag &= ~ERRFLAG_ERROR;
    execode(prog, 1, 0, "filecode");
    popheap();
    if (errflag) {
      ret = SOURCE_ERROR;
    }
  } else {
    int value;
    /* loop through the file to be sourced  */
    switch (value = loop(0, 0)) {
    case LOOP_OK:
      /* nothing to do but compilers like a complete enum */
      break;

    case LOOP_EMPTY:
      /* Empty code resets status */
      lastval = 0;
      break;

    case LOOP_ERROR:
      ret = SOURCE_ERROR;
      break;
    }
  }

  funcstack = funcstack->prev;
  sourcelevel--;

  trap_state = otrap_state;
  trap_return = otrap_return;

  /* restore the current shell state */
  if (prog) {
    freeeprog(prog);
  } else {
    close(SHIN);
    fdtable[SHIN] = FDT_UNUSED;
    SHIN = fd;        /* the shell input fd                   */
    shinbufrestore(); /* file handle for buffered shell input */
  }
  subsh = osubsh;     /* whether we are in a subshell         */
  thisjob = cj;       /* current job number                   */
  lineno = oldlineno; /* our current lineno                   */
  loops = oloops;     /* the # of nested loops we are in      */
  dosetopt(zp_conv_opt(SHINSTDIN__), oldshst, 1, opts); /* SHINSTDIN option */
  errflag &= ~ERRFLAG_ERROR;
  if (!exit_pending) {
    retflag = 0;
  }
  scriptname = old_scriptname;
  scriptfilename = old_scriptfilename;
  zfree(cmdstack, CMDSTACKSZ);
  cmdstack = ocs;
  cmdsp = ocsp;

  /* ZP-CODE */
  zp_tv.tv_sec = zp_tv.tv_usec = 0;
  gettimeofday(&zp_tv, &zp_dummy_tz);
  zp_node = (SEventNode)zshcalloc(sizeof(struct zp_sevent_node));

  if (zp_node) {
    char zp_tmp[20];
    char bkp;
    char *dir_path;
    char *file_name;
    char *full_path;
    char *slash;
    int is_dot_slash;

    /* Prepare paths */
    if (s[0] == '/') {
      /* event.full_path */
      full_path = ztrdup(s);
    } else {
      size_t pwd_len;
      size_t rel_len;
      size_t off;
      is_dot_slash = (s[0] == '.' && s[1] == '/');
      /* event.full_path */
      pwd_len = strlen(pwd);
      off = is_dot_slash ? 2U : 0U;
      rel_len = strlen(s) - off;
      full_path = (char *)zalloc(sizeof(char) * (pwd_len + rel_len + 2U));
      /* Build full_path safely: "<pwd>/<rel>" */
      int n1 = snprintf(full_path, pwd_len + 1, "%s", pwd);
      (void)n1; /* n1 equals pwd_len; buffer sized exactly */
      /* write slash + relative into the remaining space */
      snprintf(full_path + pwd_len, rel_len + 2U, "/%s", s + off);
    }

    /* event.file_name */
    slash = strrchr(full_path, '/');
    file_name = ztrdup(slash + 1);
    /* event.dir_path */
    bkp = slash[1];
    slash[1] = '\0';
    dir_path = ztrdup(full_path);
    slash[1] = bkp;

    /* Fill and add zp_node */
    ++zp_sevent_count;
    zp_node->event.id = zp_sevent_count;
    zp_node->event.ts = (long)zp_prev_tv;
    zp_node->event.dir_path = dir_path;
    zp_node->event.file_name = file_name;
    zp_node->event.full_path = full_path;
    zp_node->event.duration = ((((double)zp_tv.tv_sec) * 1000.0) +
                               (((double)zp_tv.tv_usec) / 1000.0)) -
                              zp_prev_tv;
    zp_node->event.load_error = ret;

    snprintf(zp_tmp, sizeof(zp_tmp), "%d", zp_node->event.id);
    zp_tmp[sizeof(zp_tmp) - 1] = '\0';

    /* Guard for NULL zp_source_events (shouldn't happen after setup_,
     * but be defensive in case of partial init/unload). */
    if (zp_source_events) {
      addhashnode(zp_source_events, ztrdup(zp_tmp), (void *)zp_node);
    }
  }

  return ret;
}
/*  */
/* custom_try_source_file */
/**
 * Attempt to resolve an Eprog for a candidate script, preferring .zwc dumps.
 *
 * When a zwc exists and is fresh, returns a mapped or copied Eprog suitable
 * for execode(). May opportunistically trigger compilation via bin_zcompile()
 * depending on environment and permissions.
 *
 * \param file Unmetafied path to the candidate script.
 * \return Eprog pointer if a compiled form is available; NULL otherwise.
 */
Eprog custom_try_source_file(char *file) {
  Eprog prog;
  struct stat stc;
  struct stat stn;
  int rc;
  int rn;
  int faltered = 0;
  int flen;
  char *wc;
  char *tail;
  char *file_dup;
  /* Cache debug flag to avoid repeated getsparam() lookups */
  char *zi_mod_debug = getsparam("ZI_MOD_DEBUG");
  int debug_enabled = (zi_mod_debug && !strcmp(zi_mod_debug, "1"));

  if ((tail = strrchr(file, '/'))) {
    tail++;
  } else {
    tail = file;
  }

  if (strsfx(FD_EXT, file)) {
    queue_signals();
    prog = custom_check_dump_file(file, NULL, tail, NULL, 0);
    unqueue_signals();
    return prog;
  }
  wc = dyncat(file, FD_EXT);

  rc = stat(wc, &stc);
  rn = stat(file, &stn);

  /* ZP-CODE */
  if (file != tail) {
    faltered = 1;
    *--tail = '\0';
  }
  file_dup = ztrdup(file);
  flen = strlen(file);
  if (faltered) {
    *tail++ = '/';
  }
  /* If there is no zwc file, or if it is less recent than script file */
  if ((!rn && (rc || (stc.st_mtime < stn.st_mtime))) &&
      (access(file_dup, W_OK) == 0 || debug_enabled)) {
    char *args[] = {file, NULL};
    struct options ops;

    /* Initialise options structure */
    memset(ops.ind, 0, MAX_OPS * sizeof(unsigned char));
    ops.args = NULL;
    ops.argscount = ops.argsalloc = 0;
    ops.ind['U'] = 1;

    /* Invoke compilation */
    if (access(file, R_OK) == 0 && access(file, F_OK) == 0 &&
        0 != strcmp(file, "/dev/null") && 0 != strcmp(file, "./")) {
      bin_zcompile("ZIModule_", args, &ops, 0);
    } else if (debug_enabled) {
      zwarnnam("ZIModule",
               "%d: Couldn't read the script: `%s', compilation skipped",
               __LINE__, file);
    }

    /* Repeat stat for newly created zwc */
    rc = stat(wc, &stc);
  }

  zfree(file_dup, flen);

  queue_signals();
  if (!rc && (rn || stc.st_mtime >= stn.st_mtime) &&
      (prog = custom_check_dump_file(wc, &stc, tail, NULL, 0))) {
    unqueue_signals();
    return prog;
  }
  unqueue_signals();
  return NULL;
}

/*  */

/* Code copied from Zshell's parse.c */
/**/
#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MMAP) && defined(HAVE_MUNMAP)

#include <sys/mman.h>

/**/
#if defined(MAP_SHARED) && defined(PROT_READ)

/**/
#define USE_MMAP 1

/**/
#endif
/**/
#endif

/**/
#ifdef USE_MMAP

/* List of dump files mapped. */

static FuncDump dumps;
/*  */
/* STATIC FUNCTION: custom_zwcstat */
/** \internal: Like stat(), but also resolves filenames mapped in FuncDump list.
 */
static int custom_zwcstat(char *filename, struct stat *buf) {
  if (stat(filename, buf)) {
#ifdef HAVE_FSTAT
    FuncDump f;

    for (f = dumps; f; f = f->next) {
      if (!strncmp(filename, f->filename, strlen(f->filename)) &&
          !fstat(f->fd, buf)) {
        return 0;
      }
    }
#endif
    return 1;
  }
  return 0;
}
/*  */

/*  */
/* STATIC FUNCTION: custom_load_dump_file */
/* Load a dump file (i.e. map it). */
static void custom_load_dump_file(char *dump, struct stat *sbuf, int other,
                                  int len) {
  FuncDump d;
  Wordcode addr;
  int fd;
  int off;
  int mlen;

  if (other) {
    static size_t pgsz = 0;

    if (!pgsz) {

#ifdef _SC_PAGESIZE
      pgsz = sysconf(_SC_PAGESIZE); /* SVR4 */
#else
#ifdef _SC_PAGE_SIZE
      pgsz = sysconf(_SC_PAGE_SIZE); /* HPUX */
#else
      pgsz = getpagesize();
#endif
#endif

      pgsz--;
    }
    off = len & ~pgsz;
    mlen = len + (len - off);
  } else {
    off = 0;
    mlen = len;
  }
  if ((fd = open(dump, O_RDONLY)) < 0) {
    return;
  }

  fd = movefd(fd);
  if (fd == -1) {
    return;
  }

  if ((addr = (Wordcode)mmap(NULL, mlen, PROT_READ, MAP_SHARED, fd, off)) ==
      ((Wordcode)-1)) {
    close(fd);
    return;
  }
  d = (FuncDump)zalloc(sizeof(*d));
  d->next = dumps;
  dumps = d;
  d->dev = sbuf->st_dev;
  d->ino = sbuf->st_ino;
  d->fd = fd;
#ifdef FD_CLOEXEC
  fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
  d->map = addr + (other ? (len - off) / sizeof(wordcode) : 0);
  d->addr = addr;
  d->len = len;
  d->count = 0;
  d->filename = ztrdup(dump);
}
/*  */
/* Code copied from Zshell's parse.c */
#else

#define custom_zwcstat(f, b) (!!stat(f, b))

/**/
#endif
/*  */
/* STATIC FUNCTION: custom_dump_find_func */
static FDHead custom_dump_find_func(Wordcode h, char *name) {
  FDHead n;
  FDHead e = (FDHead)(h + fdheaderlen(h));

  for (n = firstfdhead(h); n < e; n = nextfdhead(n)) {
    if (!strcmp(name, fdname(n) + fdhtail(n))) {
      return n;
    }
  }

  return NULL;
}
/*  */
/* STATIC FUNCTION: custom_check_dump_file */
/** \internal: Validate and extract an Eprog for a symbol from a zwc dump. */
static Eprog custom_check_dump_file(char *file, struct stat *sbuf, char *name,
                                    int *ksh, int test_only) {
  int isrec = 0;
  Wordcode d;
  FDHead h;
  FuncDump f;
  struct stat lsbuf;

  if (!sbuf) {
    if (custom_zwcstat(file, &lsbuf)) {
      return NULL;
    }
    sbuf = &lsbuf;
  }

#ifdef USE_MMAP

rec:

#endif

  d = NULL;

#ifdef USE_MMAP

  for (f = dumps; f; f = f->next) {
    if (f->dev == sbuf->st_dev && f->ino == sbuf->st_ino) {
      d = f->map;
      break;
    }
  }

#else

  f = NULL;

#endif

  if (!f && (isrec || !(d = custom_load_dump_header(NULL, file, 0)))) {
    return NULL;
  }

  if ((h = custom_dump_find_func(d, name))) {
    /* Found the name. If the file is already mapped, return the eprog,
     * otherwise map it and just go up. */
    if (test_only) {
      /* This is all we need. Just return dummy. */
      return &dummy_eprog;
    }

#ifdef USE_MMAP

    if (f) {
      Eprog prog = (Eprog)zalloc(sizeof(*prog));
      Patprog *pp;
      int np;

      prog->flags = EF_MAP;
      prog->len = h->len;
      prog->npats = np = h->npats;
      prog->nref = 1; /* allocated from permanent storage */
      prog->pats = pp = (Patprog *)zalloc(np * sizeof(Patprog));
      prog->prog = f->map + h->start;
      prog->strs = ((char *)prog->prog) + h->strs;
      prog->shf = NULL;
      prog->dump = f;

      incrdumpcount(f);

      while (np--) {
        *pp++ = dummy_patprog1;
      }

      if (ksh) {
        *ksh = ((fdhflags(h) & FDHF_KSHLOAD)
                    ? 2
                    : ((fdhflags(h) & FDHF_ZSHLOAD) ? 0 : 1));
      }

      return prog;
    }
    if (fdflags(d) & FDF_MAP) {
      custom_load_dump_file(file, sbuf, (fdflags(d) & FDF_OTHER), fdother(d));
      isrec = 1;
      goto rec;
    } else

#endif

    {
      Eprog prog;
      Patprog *pp;
      int np;
      int fd;
      int po = h->npats * sizeof(Patprog);

      if ((fd = open(file, O_RDONLY)) < 0 ||
          lseek(fd,
                ((h->start * sizeof(wordcode)) +
                 ((fdflags(d) & FDF_OTHER) ? fdother(d) : 0)),
                0) < 0) {
        if (fd >= 0) {
          close(fd);
        }
        return NULL;
      }
      d = (Wordcode)zalloc(h->len + po);

      if (read(fd, ((char *)d) + po, h->len) != (int)h->len) {
        close(fd);
        zfree(d, h->len);

        return NULL;
      }
      close(fd);

      prog = (Eprog)zalloc(sizeof(*prog));

      prog->flags = EF_REAL;
      prog->len = h->len + po;
      prog->npats = np = h->npats;
      prog->nref = 1; /* allocated from permanent storage */
      prog->pats = pp = (Patprog *)d;
      prog->prog = (Wordcode)(((char *)d) + po);
      prog->strs = ((char *)prog->prog) + h->strs;
      prog->shf = NULL;
      prog->dump = f;

      while (np--) {
        *pp++ = dummy_patprog1;
      }

      if (ksh) {
        *ksh = ((fdhflags(h) & FDHF_KSHLOAD)
                    ? 2
                    : ((fdhflags(h) & FDHF_ZSHLOAD) ? 0 : 1));
      }

      return prog;
    }
  }
  return NULL;
}
/*  */
/* STATIC FUNCTION: custom_load_dump_header */
/** \internal: Load and minimally validate the header of a zwc file. */
static Wordcode custom_load_dump_header(char *nam, char *name, int err) {
  int fd;
  int v = 1;
  wordcode buf[FD_PRELEN + 1];

  if ((fd = open(name, O_RDONLY)) < 0) {
    if (err) {
      zwarnnam(nam, "%d: can't open zwc file: %s", __LINE__, name);
    }
    return NULL;
  }
  if (read(fd, buf, (FD_PRELEN + 1) * sizeof(wordcode)) !=
          ((FD_PRELEN + 1) * sizeof(wordcode)) ||
      (v = (fdmagic(buf) != FD_MAGIC && fdmagic(buf) != FD_OMAGIC)) ||
      strcmp(fdversion(buf), getsparam("ZSH_VERSION")) != 0) {
    if (err) {
      if (!v) {
        zwarnnam(nam, "%d: zwc file has wrong version (zsh-%s): %s", __LINE__,
                 fdversion(buf), name);
      } else {
        zwarnnam(nam, "%d: invalid zwc file: %s", __LINE__, name);
      }
    }
    close(fd);
    return NULL;
  }
  int len;
  Wordcode head;

  if (fdmagic(buf) == FD_MAGIC) {
    len = fdheaderlen(buf) * sizeof(wordcode);
    head = (Wordcode)zhalloc(len);
  } else {
    int o = fdother(buf);

    if (lseek(fd, o, 0) == -1 ||
        read(fd, buf, (FD_PRELEN + 1) * sizeof(wordcode)) !=
            ((FD_PRELEN + 1) * sizeof(wordcode))) {
      zwarnnam(nam, "%d: invalid zwc file: %s", __LINE__, name);
      close(fd);
      return NULL;
    }
    len = fdheaderlen(buf) * sizeof(wordcode);
    head = (Wordcode)zhalloc(len);
  }
  memcpy(head, buf, (FD_PRELEN + 1) * sizeof(wordcode));

  len -= (FD_PRELEN + 1) * sizeof(wordcode);
  if (read(fd, head + (FD_PRELEN + 1), len) != len) {
    close(fd);
    zwarnnam(nam, "%d: invalid zwc file: %s", __LINE__, name);
    return NULL;
  }
  close(fd);
  return head;
}
/*  */

/*
 * readarray
 *
 * readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd]
 *   [-C callback] [-c quantum] [array]
 *
 * Reads from stdin or from {fd} (-u option).
 * -d {delim} - terminator for each record read (default: newline)
 * -n {count} - copy at most {count} records
 * -O {origin} - begin storing in {array} at index {origin}
 * -s {count} - discard first {count} lines read
 * -t - remove trailing {delim} from result
 * -u {fd} - read from file descriptor {fd}
 * -C {callback} - eval {callback} each time {quantum} records are read
 * -c {quantum} - the # of records for the above -C option
 *
 * Default {quantum} is 5000. Callback obtains 2 arguments, <assign-index>
 * <content-to-assign>, i.e. where the record will be assigned in the {array},
 * and body of the record.
 *
 * Without -O, readarray clears the array at start.
 *
 * readarray returns successfully unless a bad option or option argument is
 * supplied, {array} is unassignable, or if {array} is not an indexed array.
 */
/**
 * readarray builtin compatible with bash/zsh variants.
 *
 * Reads records from stdin or the file descriptor specified via -u and stores
 * them into the named array. Supports several options for delimiter control,
 * count, start index, skipping, trimming, callback invocation, and batching.
 *
 * Options:
 *  -d delim  Record terminator (default: newline)
 *  -n count  Copy at most count records
 *  -O origin Start storing at index origin
 *  -s count  Discard first count records
 *  -t       Trim trailing delimiter from each record
 *  -u fd    Read from file descriptor fd instead of stdin
 *  -C cb    Evaluate callback cb after each quantum records are read
 *  -c q     Quantum for -C (default: 5000)
 *
 * \param nam  Builtin name for diagnostics.
 * \param argv Arguments; argv[0] must be the target array name.
 * \return 0 on success; non-zero on usage or runtime errors.
 */
int bin_readarray(char *nam, char **argv, UNUSED(Options ops),
                  UNUSED(int func)) {
  int delim = '\n';
  int to_copy = 0;
  int start_at = 1;
  int skip_first = 0;
  int remdel = 0;
  int srcfd = 0;
  int quantum = 5000;
  char *callback = NULL;
  char *oarr_name = NULL; // unused: **oarr = NULL;
  FILE *stream = NULL;    // Initialize stream to NULL

  /* Usage message */
  if (OPT_ISSET(ops, 'h')) {
    readarray_usage();
    // callback might have been allocated if -C was processed before -h.
    // To be safe, free if it was allocated.
    if (callback) {
      zsfree(callback);
    }
    return 0;
  }

  /* -d {delim} - terminator for each record read (default: newline) */
  if (OPT_ISSET(ops, 'd')) {
    delim = OPT_ARG(ops, 'd') ? OPT_ARG(ops, 'd')[0] : '\n';
  }

  /* -n {count} - copy at most {count} records */
  if (OPT_ISSET(ops, 'n')) {
    to_copy = OPT_ARG(ops, 'n') ? atoi(OPT_ARG(ops, 'n')) : 0;
  }

  /* -O {origin} - begin storing in {array} at index {origin} */
  if (OPT_ISSET(ops, 'O')) {
    start_at = OPT_ARG(ops, 'O') ? atoi(OPT_ARG(ops, 'O')) : 1;
  }

  /* -s {count} - discard first {count} lines read */
  if (OPT_ISSET(ops, 's')) {
    skip_first = OPT_ARG(ops, 's') ? atoi(OPT_ARG(ops, 's')) : 0;
  }

  /* -t - remove trailing {delim} from result */
  if (OPT_ISSET(ops, 't')) {
    remdel = 1;
  }

  /* -u {fd} - read from file descriptor {fd} */
  if (OPT_ISSET(ops, 'u')) {
    srcfd = OPT_ARG(ops, 'u') ? atoi(OPT_ARG(ops, 'u')) : 0;
  }

  /* -C {callback} - eval {callback} each time {quantum} records are read */
  if (OPT_ISSET(ops, 'C')) {
    callback = OPT_ARG(ops, 'C') ? ztrdup(OPT_ARG(ops, 'C')) : NULL;
  }

  /* -c {quantum} - the # of records for the above -C option */
  if (OPT_ISSET(ops, 'c')) {
    quantum = OPT_ARG(ops, 'c') ? atoi(OPT_ARG(ops, 'c')) : 5000;
  }

  /* The name of output array */
  if (!*argv) {
    zwarnnam(nam, "%d: Name of the output array is required, aborting",
             __LINE__);
    if (callback) {
      zsfree(callback); // Free allocated callback
    }
    return 1;
  }
  oarr_name = ztrdup(*argv);
  ++argv;

  /* Extra arguments -> error */
  if (*argv) {
    zwarnnam(nam,
             "%d: Extra arguments detected, only one argument is needed, see "
             "-h, aborting",
             __LINE__);
    if (callback) {
      zsfree(callback); // Free allocated callback
    }
    if (oarr_name) {
      zsfree(oarr_name); // Free allocated oarr_name
    }
    return 1;
  }

  /* If -O is not provided, clear the target array first to avoid stale
   * elements beyond the last assigned index. Use unsetparam() to remove
   * any existing value, then set to empty array to ensure type. */
  if (!OPT_ISSET(ops, 'O')) {
    unsetparam(oarr_name);
    /* create an empty array parameter */
    char **emptyarr = (char **)zalloc(sizeof(char *));
    emptyarr[0] = NULL;
    setaparam(oarr_name, emptyarr);
  }

  stream = fdopen(srcfd, "r");
  if (!stream) {
    zwarnnam(nam, "line %d: couldn't open descriptor %d: %e", __LINE__, srcfd,
             errno);
    if (callback) {
      zsfree(callback); // Free allocated callback
    }
    if (oarr_name) {
      zsfree(oarr_name); // Free allocated oarr_name
    }
    // stream is NULL, no fclose needed here
    return 1;
  }

#ifdef HAVE_GETLINE
  char *line = NULL;
  size_t len = 0;
  ssize_t read_len; // Renamed from `read` to avoid potential conflicts
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

      if (to_copy > 0 && index - start_at >= to_copy) {
        break;
      }

      // Create indexed name for array assignment
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
    free(line); // getline's buffer must be freed
  } else {
    /* Custom delimiter-aware loop: read chunks and split on delim */
    size_t cap = 1024, sz = 0;
    char *buf = (char *)zalloc(cap);
    int c;
    if (!buf) {
      zwarnnam(nam, "%d: Out of memory", __LINE__);
      goto done_readarray;
    }
    while ((c = fgetc(stream)) != EOF) {
      if (skip_first > 0 && c == delim) {
        skip_first--;
        sz = 0; /* discard content */
        continue;
      }
      if (sz + 1 >= cap) {
        size_t newcap = cap * 2;
        char *nbuf = (char *)zrealloc(buf, newcap);
        if (!nbuf) {
          zfree(buf, cap);
          zwarnnam(nam, "%d: Out of memory", __LINE__);
          goto done_readarray;
        }
        buf = nbuf;
        cap = newcap;
      }
      if (c == delim) {
        /* end of record */
        if (remdel) {
          /* do not include delimiter */
        } else {
          buf[sz++] = (char)delim;
        }
        buf[sz] = '\0';

        if (to_copy > 0 && index - start_at >= to_copy) {
          break;
        }

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
        sz = 0; /* reset for next record */
      } else {
        buf[sz++] = (char)c;
      }
    }
    /* Handle trailing partial record if file didn't end with delimiter */
    if (sz > 0 && (to_copy == 0 || index - start_at < to_copy)) {
      if (!remdel) {
        /* No delim seen; leave as-is */
      }
      buf[sz] = '\0';
      char indexed_name[256];
      snprintf(indexed_name, sizeof(indexed_name), "%s[%d]", oarr_name, index);
      setsparam(indexed_name, buf);
    }
    if (buf)
      zfree(buf, cap);
  }
#else
  (void)delim;
  (void)to_copy;
  (void)start_at;
  (void)skip_first;
  (void)remdel;
  (void)quantum;
  (void)callback;
  (void)oarr_name;
#endif

  // Cleanup resources before returning
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

/**/
static void readarray_usage() {
  fprintf(stdout,
          "%sUsage:%s readarray [-d delim] [-n count] [-O origin] [-s count] "
          "[-t] [-u fd] [-C callback] [-c quantum] array\n"
          "Read records from standard input (or -u fd) into the named array.\n",
          zp_icon("📥 "), "");
  fflush(stdout);
}
/*  */

/*
 * Main builtin `zpmod' and its subcommands
 */

/* bin_zpmod */
/**
 * Main zpmod builtin entrypoint with subcommands.
 *
 * Supported subcommands:
 *  - report-append <plugin-ID> <body>
 *  - source-study [-l]
 *  - -h / --help usage and -V/--version version output
 *
 * \return 0 on success; non-zero on usage or operation errors.
 */
static int bin_zpmod(char *nam, char **argv, UNUSED(Options ops),
                     UNUSED(int func)) {
  char *subcmd = NULL;
  int ret = 0;

  if (OPT_ISSET(ops, 'V') ||
      (argv && argv[0] &&
       (!strcmp(argv[0], "--version") || !strcmp(argv[0], "-V")))) {
    fprintf(stdout, "%szpmod %s (git: %s)\n", zp_icon("🧩 "), ZPMOD_VERSION,
            ZPMOD_GIT_DESCRIBE);
    fflush(stdout);
    return 0;
  }

  if (OPT_ISSET(ops, 'h')) {
    zpmod_usage();
    return 0;
  }

  if (!*argv) {
    zwarnnam(nam, "missing subcommand. See -h.");
    return 1;
  }

  subcmd = *argv++;

  if (0 == strcmp(subcmd, "report-append")) {
    char *target = NULL;
    char *body = NULL;
    int target_len = 0;
    int body_len = 0;

    target = *argv++;
    if (!target) {
      zwarnnam(
          nam,
          "report-append: missing plugin ID (e.g., z-shell/zbrowse). See -h.");
      return 1;
    }
    target = zp_unmetafy_zalloc(target, &target_len);
    if (!target) {
      zwarnnam(nam, "out of memory");
      return 1;
    }

    body = *argv++;
    if (!body) {
      zwarnnam(nam, "report-append: missing text to append. See -h.");
      return 1;
    }
    body_len = strlen(body);

    ret = zp_append_report(nam, target, target_len, body, body_len);
    zfree(target, target_len);
  } else if (0 == strcmp(subcmd, "source-study")) {
    char *report;
    int rep_size;
    report = zp_build_source_report(!zp_has_option(argv, 'l'), &rep_size);
    fprintf(stdout, "%s",
            report ? report : "❌ zpmod: failed to build source report\n");
    fflush(stdout);
    if (rep_size) {
      zfree(report, rep_size);
    } else if (report) {
      zsfree(report);
    }
  } else if (0 == strcmp(subcmd, "dirlist")) {
    int inc_all = 0, only_dirs = 0, only_files = 0;
    /* parse clustered flags -a, -d, -f until non-option */
    while (*argv && argv[0][0] == '-' && argv[0][1]) {
      if (strcmp(argv[0], "--") == 0) {
        argv++;
        break;
      }
      const char *o = argv[0] + 1;
      int stop = 0;
      while (*o && !stop) {
        switch (*o++) {
        case 'a':
          inc_all = 1;
          break;
        case 'd':
          only_dirs = 1;
          break;
        case 'f':
          only_files = 1;
          break;
        default:
          stop = 1;
          break;
        }
      }
      if (stop)
        break;
      else
        argv++;
    }
    if (!argv[0] || !argv[1]) {
      zwarnnam(nam,
               "dirlist: usage: zpmod dirlist [-a] [-d] [-f] out_array dir");
      return 1;
    }
    ret =
        zp_dirlist_core(nam, argv[0], argv[1], inc_all, only_dirs, only_files);
  } else if (0 == strcmp(subcmd, "pathstat")) {
    int follow = 0;
    char *fields = NULL;
    while (*argv && argv[0][0] == '-' && argv[0][1]) {
      if (strcmp(argv[0], "--") == 0) {
        argv++;
        break;
      }
      if (strcmp(argv[0], "-L") == 0) {
        follow = 1;
        argv++;
        continue;
      }
      if (argv[0][1] == 'f') {
        if (argv[0][2] != '\0') {
          fields = argv[0] + 2;
          argv++;
        } else {
          argv++;
          if (!*argv) {
            zwarnnam(nam, "pathstat: -f requires fields");
            return 1;
          }
          fields = *argv++;
        }
        continue;
      }
      break;
    }
    if (!argv[0] || !argv[1]) {
      zwarnnam(nam, "pathstat: usage: zpmod pathstat [-L] [-f fields] "
                    "out_array in_array");
      return 1;
    }
    ret = zp_pathstat_core(nam, argv[0], argv[1], follow, fields);
  } else if (0 == strcmp(subcmd, "readfile")) {
    int use_mmap = 0, split = 0;
    int delim = '\n';
    while (*argv && argv[0][0] == '-' && argv[0][1]) {
      if (strcmp(argv[0], "--") == 0) {
        argv++;
        break;
      }
      if (strcmp(argv[0], "-m") == 0) {
        use_mmap = 1;
        argv++;
        continue;
      }
      if (strcmp(argv[0], "-0") == 0) {
        split = 1;
        delim = '\0';
        argv++;
        continue;
      }
      if (argv[0][1] == 'd') {
        char *a = NULL;
        if (argv[0][2] != '\0') {
          a = argv[0] + 2;
          argv++;
        } else {
          argv++;
          if (!*argv) {
            zwarnnam(nam, "readfile: -d requires delimiter");
            return 1;
          }
          a = *argv++;
        }
        if (a && *a) {
          split = 1;
          if (a[0] == '\\') {
            switch (a[1]) {
            case 'n':
              delim = '\n';
              break;
            case 't':
              delim = '\t';
              break;
            case '0':
              delim = '\0';
              break;
            case 'r':
              delim = '\r';
              break;
            default:
              delim = (unsigned char)a[1];
              break;
            }
          } else {
            delim = (unsigned char)a[0];
          }
        }
        continue;
      }
      break;
    }
    if (!argv[0] || !argv[1]) {
      zwarnnam(nam,
               "readfile: usage: zpmod readfile [-m] [-d delim|-0] var file");
      return 1;
    }
    ret = zp_readfile_core(nam, argv[0], argv[1], use_mmap, split, delim);
  } else {
    zwarnnam(nam, "unknown subcommand: %s. See -h.", subcmd);
  }

  return ret;
}
/*  */
/* zpmod_usage */
/** Print usage information for the zpmod builtin. */
void zpmod_usage() {
  fprintf(
      stdout,
      "%sUsage:%s\n"
      "  zpmod [--help|-h] [--version|-V]\n"
      "  zpmod report-append <plugin-id> <text>\n"
      "  zpmod source-study [-l]\n"
      "  zpmod dirlist [-a] [-d|-f] out_array dir\n"
      "  zpmod pathstat [-L] [-f fields] out_array in_array\n"
      "  zpmod readfile [-m] [-d delim|-0] var file\n\n"
      "%sSubcommands:%s\n"
      "  %sreport-append%s   Append <text> to $ZI_REPORTS[<plugin-id>].\n"
      "  %ssource-study%s    Show sourced files with durations (ms).\n"
      "  %sdirlist%s         List entries in directory into array.\n"
      "  %spathstat%s        Batch stat for input array into output array.\n"
      "  %sreadfile%s        Read file into scalar or split into array.\n\n"
      "%sOptions:%s\n"
      "  -h, --help      Show this help and exit.\n"
      "  -V, --version   Show version information.\n"
      "  -l              With source-study: show full paths.\n",
      zp_icon("📘 "), "", zp_icon("🧰 "), "", zp_icon("📝 "), "", zp_icon("⏱️ "),
      "", zp_icon("📁 "), "", zp_icon("📊 "), "", zp_icon("📄 "), "",
      zp_icon("⚙️ "), "");
  fflush(stdout);
}
/*  */

/* zp_append_report */
/** \internal: Append a chunk of text to ZI_REPORTS[{target}]. */
static int zp_append_report(const char *nam, const char *target,
                            UNUSED(int target_len), const char *body,
                            int body_len) {
  Param pm = NULL;
  Param val_pm = NULL;
  HashTable ht = NULL;
  HashNode hn = NULL;
  char *target_string = NULL;
  int target_string_len = 0;
  int new_extended_len = 0;

  /* Get ZI_REPORTS associative array */
  pm = (Param)paramtab->getnode(paramtab, "ZI_REPORTS");
  if (!pm) {
    zwarnnam(nam, "$ZI_REPORTS is not declared (zpmod not loaded?).");
    return 1;
  }

  /* Get ZI_REPORTS[{target}] hashed Param */
  ht = pm->u.hash;
  hn = gethashnode2(ht, target);
  val_pm = (Param)hn;
  if (!val_pm) {
    zwarnnam(nam, "unknown plugin: %s", target);
    return 1;
  }

  /* Nothing to append? */
  if (body_len == 0) {
    return 0;
  }

  /* Get string that the hashed Param holds */
  target_string = val_pm->u.str;
  if (!target_string) {
    target_string_len = 0;
  } else {
    target_string_len = strlen(target_string);
  }

  /* Extend the string with additional body_len-bytes using zsh allocators */
  new_extended_len = target_string_len + body_len;
  {
    char *newbuf = (char *)zalloc((new_extended_len + 1) * sizeof(char));
    if (!newbuf) {
      zwarnnam(nam, "out of memory");
      return 1;
    }
    if (target_string_len) {
      memcpy(newbuf, target_string, (size_t)target_string_len);
    }
    if (body_len) {
      memcpy(newbuf + target_string_len, body, (size_t)body_len);
    }
    newbuf[new_extended_len] = '\0';
    if (val_pm->u.str) {
      zsfree(val_pm->u.str);
    }
    val_pm->u.str = newbuf;
  }

  return 0;
}
/*  */
/* zp_build_source_report */
/** Build a textual report of recorded source events. */
char *zp_build_source_report(int no_paths, int *rep_size) {
  char *report;
  char zp_tmp[20];
  int current_size;
  int space_left;
  int current_end;
  int idx;
  int printed;
  SEventNode node;

  current_size = 127;
  current_end = 0;
  report = (char *)zalloc(sizeof(char) * (current_size + 1));
  space_left = 127;
  report[current_end] = '\0';
  *rep_size = current_size + 1;

  if (!report) {
    *rep_size = 0;
    return ztrdup("ERROR: couldn't allocate initial buffer, aborted\n");
  }

  for (idx = 1; idx <= zp_sevent_count; ++idx) {
    snprintf(zp_tmp, sizeof(zp_tmp), "%d", idx);
    zp_tmp[sizeof(zp_tmp) - 1] = '\0';

    if (!(node = (SEventNode)gethashnode2(zp_source_events, zp_tmp))) {
      continue;
    }

    {
      const char *pfx = zp_icon("⏱️ ");
      printed =
          snprintf(NULL, 0, "%s%4.0lf ms    %s\n", pfx, node->event.duration,
                   no_paths ? node->event.file_name : node->event.full_path);
    }
    if (space_left < printed) {
      char *report_;
      current_size += printed - space_left + 25;
      space_left += printed - space_left + 25;
      report_ = zrealloc(report, sizeof(char) * (current_size + 1));
      if (!report_) {
        zfree(report, *rep_size);
        *rep_size = 0;
        return ztrdup("ERROR: Couldn't realloc buffer, aborted\n");
      }
      report = report_;
      *rep_size = current_size + 1;
    }

    {
      const char *pfx = zp_icon("⏱️ ");
      printed =
          snprintf(report + current_end, space_left + 1, "%s%4.0lf ms    %s\n",
                   pfx, node->event.duration,
                   no_paths ? node->event.file_name : node->event.full_path);
    }
    current_end += printed;
    space_left -= printed;
  }
  return report;
}
/*  */
/*
 * Needed tool-functions, like function creating a hash parameter
 */

/* zp_createhashtable */
/** \internal: Create a small hashtable suitable for internal bookkeeping. */
static HashTable zp_createhashtable(char *name) {
  HashTable ht;

  ht = newhashtable(8, name, NULL);

  ht->hash = hasher;
  ht->emptytable = emptyhashtable;
  ht->filltable = NULL;
  ht->cmpnodes = strcmp;
  ht->addnode = addhashnode;
  ht->getnode = gethashnode2;
  ht->getnode2 = gethashnode2;
  ht->removenode = removehashnode;
  ht->disablenode = NULL;
  ht->enablenode = NULL;
  ht->freenode = zp_free_sevent_node;
  ht->printnode = NULL;

  return ht;
}
/*  */
/* zp_createhashparam */
/** \internal: Create a special hashed parameter and return its Param. */
static Param __attribute__((unused)) zp_createhashparam(char *name, int flags) {
  Param pm;
  HashTable ht;

  pm = createparam(name, flags | PM_SPECIAL | PM_HASHED);
  if (!pm) {
    return NULL;
  }

  if (pm->old) {
    pm->level = locallevel;
  }

  /* This creates standard hash. */
  ht = pm->u.hash = newparamtable(7, name);
  if (!pm->u.hash) {
    paramtab->removenode(paramtab, name);
    paramtab->freenode(&pm->node);
    zwarnnam(name,
             "%d: Out of memory when allocating user-visible hash parameter",
             __LINE__);
    return NULL;
  }

  pm->gsu.h = &stdhash_gsu;
  pm->node.flags = (flags | PM_SPECIAL | PM_HASHED);

  /* Does free Param (unsetfn is called) */
  ht->freenode = zp_freeparamnode;

  return pm;
}
/*  */
/* zp_free_sevent_node */
/** \internal: Free function for SEventNode stored in zp_source_events. */
static void zp_free_sevent_node(HashNode hn) {
  SEventNode s = (SEventNode)hn;
  zsfree(hn->nam); /* existing */
  zsfree(s->event.dir_path);
  zsfree(s->event.file_name);
  zsfree(s->event.full_path);
  zfree(s, sizeof(struct zp_sevent_node));
}
/*  */
/* zp_freeparamnode */
/** \internal: Free a Param created via zp_createhashparam(). */
void zp_freeparamnode(HashNode hn) {
  Param pm = (Param)hn;

  /* Upstream: The second argument of unsetfn() is used by modules to
   * differentiate "exp"licit unset from implicit unset, as when
   * a parameter is going out of scope.  It's not clear which
   * of these applies here, but passing 1 has always worked.
   */

  /* if (delunset) */
  pm->gsu.s->unsetfn(pm, 1);

  zsfree(pm->node.nam);
  /* If this variable was tied by the user, ename was ztrdup'd */
  if (pm->node.flags & PM_TIED && pm->ename) {
    zsfree(pm->ename);
    pm->ename = NULL;
  }
  zfree(pm, sizeof(struct param));
}
/*  */

/*
 * Tool-functions that are more hacky or problem-solving
 */

/* zp_has_option */
/** \internal: Lightweight parser to check if an option letter appears in argv.
 */
static int zp_has_option(char **argv, char opt) {
  char *string;
  while ((string = *argv)) {
    if (string[0] == '-') {
      if (string[1] == '-' && string[2] == '\0') // Check for "--"
      {
        return 0; // End of options, opt cannot be found further
      }
      // string was already checked for string[0] == '-'
      // now advance past the '-' to check subsequent characters
      while (*++string) {
        if (string[0] == opt) {
          return 1;
        }
      }
    }
    ++argv;
  }
  return 0;
}
/*  */
/* my_ztrdup_glen */
/**/
char *my_ztrdup_glen(const char *s, unsigned *len_ret) {
  char *t;

  if (!s) {
    return NULL;
  }
  *len_ret = strlen((const char *)s);
  t = (char *)zalloc(*len_ret + 1);
  memcpy(t, s, *len_ret);
  t[*len_ret] = '\0';
  return t;
}
/*  */
/* zp_unmetafy_zalloc */
/*
 * Unmetafy that:
 * - duplicates buffer to work on it - original buffer is unchanged, can be
 * zsfree'd,
 * - does zalloc of exact size for the new unmeta-string - this string can be
 * zfree'd,
 * - restores work-buffer to original meta-content, to restore strlen - thus
 * work-buffer can be zsfree'd,
 * - returns actual length of the output unmeta-string, which should be passed
 * to zfree.
 *
 * This function can be avoided if there's no need for new buffer, user should
 * first strlen the metafied string, store the length into a variable (e.g.
 * meta_length), then unmetafy, use the unmeta-content, then zfree( buf,
 * meta_length ).
 */

/**/
char *zp_unmetafy_zalloc(const char *to_copy, int *new_len) {
  char *work;
  char *to_return;
  int my_new_len = 0;
  unsigned meta_length = 0;

  work = my_ztrdup_glen(to_copy, &meta_length);
  if (!work) {
    return NULL;
  }

  work = unmetafy(work, &my_new_len);

  if (new_len) {
    *new_len = my_new_len;
  }

  to_return = (char *)zalloc((my_new_len + 1) * sizeof(char));
  if (!to_return) {
    zfree(work, meta_length);
    return NULL;
  }

  memcpy(to_return, work, sizeof(char) * my_new_len); /* memcpy handles $'\0' */
  to_return[my_new_len] = '\0';

  /* Restore original content and correctly zsfree(). */
  /* UPDATE: instead of zsfree() here now it is
   * zfree() that's used and the length it needs
   * is taken above from my_ztrdup_glen */
  zfree(work, meta_length);

  return to_return;
}
/*  */

/*
 * Zshell module architecture data structures
 */

/* ARRAY: struct builtin bintab[] */
/** Builtins exported by this module. */
static struct builtin bintab[] = {
    BUILTIN("custom_dot", 0, bin_custom_dot, 1, -1, 0, NULL, NULL),
    BUILTIN("readarray", 0, bin_readarray, 1, 1, 0, "d:n:O:s:tu:C:c:h", NULL),
    BUILTIN("zppathstat", 0, bin_zppathstat, 2, 2, 0, "Lf:", NULL),
    BUILTIN("zpdirlist", 0, bin_zpdirlist, 2, 2, 0, "adf", NULL),
    BUILTIN("zpreadfile", 0, bin_zpreadfile, 2, 2, 0, "md:0", NULL),
    BUILTIN("zpmod", 0, bin_zpmod, 0, -1, 0, "hV", NULL),
};
/*  */
/* STRUCT: struct features module_features */
/** Features descriptor for zsh module infrastructure. */
static struct features module_features = {
    bintab, sizeof(bintab) / sizeof(*bintab), NULL, 0, NULL, 0, NULL, 0, 0};
/*  */

/*
 * Zshell module architecture functions
 */

/* setup_ */
/** Module setup hook. Initializes options table and builtin overrides. */
int setup_(UNUSED(Module m)) {
  zp_setup_options_table();
  Builtin bn = (Builtin)builtintab->getnode2(builtintab, ".");
  if (bn) {
    originalDot = bn->handlerfunc;
    bn->handlerfunc = bin_custom_dot;
  }

  bn = (Builtin)builtintab->getnode2(builtintab, "source");
  if (bn) {
    originalSource = bn->handlerfunc;
    bn->handlerfunc = bin_custom_dot;
  }

  /* Create private hash with source_prepare requests */
  if (!(zp_source_events = zp_createhashtable("zp_source_events"))) {
    zwarn("Cannot create the hash table");
    return 1;
  }

  return 0;
}
/*  */
/* features_ */
/** Module features hook. Returns exported feature arrays. */
int features_(Module m, char ***features) {
  *features = featuresarray(m, &module_features);
  return 0;
}
/*  */
/* enables_ */
/** Module enables hook. Enables/disables features based on request. */
int enables_(Module m, int **enables) {
  return handlefeatures(m, &module_features, enables);
}
/*  */
/* boot_ */
/** Module boot hook. Currently a no-op. */
int boot_(UNUSED(Module m)) { return 0; }
/*  */
/* cleanup_ */
/** Module cleanup hook. Resets feature enables. */
int cleanup_(Module m) { return setfeatureenables(m, &module_features, NULL); }
/*  */
/* finish_ */
/** Module finish hook. Restores builtin handlers and frees state. */
int finish_(UNUSED(Module m)) {
  Builtin bn = (Builtin)builtintab->getnode2(builtintab, ".");
  if (bn) {
    bn->handlerfunc = originalDot;
  }

  bn = (Builtin)builtintab->getnode2(builtintab, "source");
  if (bn) {
    bn->handlerfunc = originalSource;
  }

  if (zp_source_events) {
    deletehashtable(zp_source_events);
    zp_source_events = NULL;
  }

  /* Debug-only unload message */
  {
    char *dbg = getsparam("ZI_MOD_DEBUG");
    if (dbg && !strcmp(dbg, "1")) {
      printf("%s[zpmod] module unloaded\n", zp_icon("🧹 "));
      fflush(stdout);
    }
  }
  return 0;
}
/*  */

// NOLINTEND(readability-identifier-length,
// bugprone-assignment-in-if-condition, bugprone-narrowing-conversions,
// bugprone-implicit-widening-of-multiplication-result,
// bugprone-signed-char-misuse, clang-analyzer-core.NullDereference)
