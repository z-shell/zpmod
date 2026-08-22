/* SPDX-License-Identifier: MIT */
/**
 * @file source.c
 * @brief Overrides for '.'/source and source-study event recording/reporting.
 *
 * This module hooks into zsh’s sourcing mechanism to optionally compile ZWC
 * caches, measure durations, and produce a report via the `zpmod source-study`
 * subcommand. It also respects user options like FUNCTION_ARGZERO, PATH_DIRS,
 * POSIX_BUILTINS, SHIN_STDIN, and SOURCE_TRACE through the stable option
 * mapping helpers.
 */
#ifndef ZPMOD_ANALYSIS
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_vendor_shims.h"
#else
#include "zpmod_analysis_stubs.h"
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(__has_include)
#if __has_include(<sys/mman.h>)
#include <sys/mman.h>
#define USE_MMAP 1
#endif
#endif
#include "zpmod_compat.h"
#include "zpmod_emoji.h"
#include "zpmod_source.h"
#include "zpmod_utils.h"

/* State */
static HandlerFunc original_dot = NULL, original_source = NULL;
static HashTable zp_source_events = NULL;
static int zp_sevent_count = 0;

/* dot/source replacement */
/**
 * @brief Replacement handler for '.' and 'source' builtins.
 *
 * Mirrors zsh’s semantics while integrating custom search behavior and
 * error reporting, and delegates to custom_source() to execute the script.
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
  if (*name != '.' && access(s, F_OK) == 0 && stat(s, &st) >= 0 &&
      !S_ISDIR(st.st_mode)) {
    diddot = 1;
    ret = custom_source(enam);
  }
  if (ret == SOURCE_NOT_FOUND) {
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
  if (argv[1]) {
    freearray(pparams);
    pparams = old;
  }
  if (ret == SOURCE_NOT_FOUND) {
    if (isset(zp_conv_opt(POSIXBUILTINS__))) {
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

/* ZWC access (trimmed) */
#define FD_EXT ".zwc"
#define FD_PRELEN 12
#define FD_MAGIC 0x04050607
#define FD_OMAGIC 0x07060504
#define FDF_MAP 1
#define FDF_OTHER 2

typedef struct fdhead *FDHead;
struct fdhead {
  wordcode start;
  wordcode len;
  wordcode npats;
  wordcode strs;
  wordcode hlen;
  wordcode flags;
};
#define FDHEADERLEN(f) (((Wordcode)(f))[FD_PRELEN])
#define FDMAGIC(f) (((Wordcode)(f))[0])
#define FDSETBYTE(f, i, v)                                                     \
  ((((unsigned char *)(((Wordcode)(f)) + 1))[i]) = ((unsigned char)(v)))
#define fdbyte(f, i) ((wordcode)(((unsigned char *)(((Wordcode)(f)) + 1))[i]))
#define FDFLAGS(f) fdbyte(f, 0)
#define FDOTHER(f) (fdbyte(f, 1) + (fdbyte(f, 2) << 8) + (fdbyte(f, 3) << 16))
#define FDVERSION(f) ((char *)((f) + 2))
#define FIRSTFDHEAD(f) ((FDHead)(((Wordcode)(f)) + FD_PRELEN))
#define NEXTFDHEAD(f) ((FDHead)(((Wordcode)(f)) + (f)->hlen))
#define FDHFLAGS(f) (((FDHead)(f))->flags)
#define FDHTAIL(f) (((FDHead)(f))->flags >> 2)
#define FDHF_KSHLOAD 1
#define FDHF_ZSHLOAD 2
#define FDNAME(f) ((char *)(((FDHead)(f)) + 1))

#ifdef USE_MMAP
static FuncDump dumps;
static int custom_zwcstat(char *filename, struct stat *buf) {
  if (stat(filename, buf)) {
#ifdef HAVE_FSTAT
    for (FuncDump fdump_iter = dumps; fdump_iter;
         fdump_iter = fdump_iter->next) {
      if (!strncmp(filename, fdump_iter->filename,
                   strlen(fdump_iter->filename)) &&
          !fstat(fdump_iter->fd, buf)) {
        return 0;
      }
    }
#endif
    return 1;
  }
  return 0;
}
#else
#define custom_zwcstat(f, b) (!!stat(f, b))
#endif

static FDHead custom_dump_find_func(Wordcode h, char *name) {
  FDHead n;
  FDHead e = (FDHead)(h + FDHEADERLEN(h));
  for (n = FIRSTFDHEAD(h); n < e; n = NEXTFDHEAD(n)) {
    if (!strcmp(name, FDNAME(n) + FDHTAIL(n))) {
      return n;
    }
  }
  return NULL;
}

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
      (v = (FDMAGIC(buf) != FD_MAGIC && FDMAGIC(buf) != FD_OMAGIC)) ||
      strcmp(FDVERSION(buf), getsparam("ZSH_VERSION")) != 0) {
    if (err) {
      if (!v) {
        zwarnnam(nam, "%d: zwc file has wrong version (zsh-%s): %s", __LINE__,
                 FDVERSION(buf), name);
      } else {
        zwarnnam(nam, "%d: invalid zwc file: %s", __LINE__, name);
      }
    }
    close(fd);
    return NULL;
  }
  int len;
  Wordcode head;
  if (FDMAGIC(buf) == FD_MAGIC) {
    len = FDHEADERLEN(buf) * sizeof(wordcode);
    head = (Wordcode)zhalloc(len);
  } else {
    int o = FDOTHER(buf);
    if (lseek(fd, o, 0) == -1 ||
        read(fd, buf, (FD_PRELEN + 1) * sizeof(wordcode)) !=
            ((FD_PRELEN + 1) * sizeof(wordcode))) {
      zwarnnam(nam, "%d: invalid zwc file: %s", __LINE__, name);
      close(fd);
      return NULL;
    }
    len = FDHEADERLEN(buf) * sizeof(wordcode);
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

#ifdef USE_MMAP
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
      pgsz = sysconf(_SC_PAGESIZE);
#else
#ifdef _SC_PAGE_SIZE
      pgsz = sysconf(_SC_PAGE_SIZE);
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
#endif

/* Forward (file-local) declaration */
static Eprog custom_check_dump_file(char *file, struct stat *sbuf, char *name,
                                    int *ksh, int test_only);

/**
 * @brief Try to locate or build a ZWC dump for the script and return Eprog.
 *
 * When possible, compiles to .zwc and loads that for performance.
 * Returns NULL if no dump is available and the caller should use a file fd.
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
  if (file != tail) {
    faltered = 1;
    *--tail = '\0';
  }
  file_dup = ztrdup(file);
  flen = strlen(file);
  if (faltered) {
    *tail++ = '/';
  }
  if ((!rn && (rc || (stc.st_mtime < stn.st_mtime))) && S_ISREG(stn.st_mode) &&
      (access(file_dup, W_OK) == 0 || debug_enabled)) {
    char *args[] = {file, NULL};
    struct options ops;
    memset(ops.ind, 0, MAX_OPS * sizeof(unsigned char));
    ops.args = NULL;
    ops.argscount = ops.argsalloc = 0;
    ops.ind['U'] = 1;
    if (access(file, R_OK) == 0 && access(file, F_OK) == 0 &&
        0 != strcmp(file, "/dev/null") && 0 != strcmp(file, "./")) {
      bin_zcompile("ZIModule_", args, &ops, 0);
    } else if (debug_enabled) {
      zwarnnam("ZIModule",
               "%d: Couldn't read the script: `%s', compilation skipped",
               __LINE__, file);
    }
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
    if (test_only) {
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
      prog->nref = 1;
      prog->pats = pp = (Patprog *)zalloc(np * sizeof(Patprog));
      prog->prog = f->map + h->start;
      prog->strs = ((char *)prog->prog) + h->strs;
      prog->shf = NULL;
      prog->dump = NULL;
      incrdumpcount(f);
      while (np--) {
        *pp++ = dummy_patprog1;
      }
      if (ksh) {
        *ksh = ((FDHFLAGS(h) & FDHF_KSHLOAD)
                    ? 2
                    : ((FDHFLAGS(h) & FDHF_ZSHLOAD) ? 0 : 1));
      }
      return prog;
    }
    if (FDFLAGS(d) & FDF_MAP) {
      custom_load_dump_file(file, sbuf, (FDFLAGS(d) & FDF_OTHER), FDOTHER(d));
      isrec = 1;
      goto rec;
    }
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
                 ((FDFLAGS(d) & FDF_OTHER) ? FDOTHER(d) : 0)),
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
      prog->nref = 1;
      prog->pats = pp = (Patprog *)d;
      prog->prog = (Wordcode)(((char *)d) + po);
      prog->strs = ((char *)prog->prog) + h->strs;
      prog->shf = NULL;
      prog->dump = f;
      while (np--) {
        *pp++ = dummy_patprog1;
      }
      if (ksh) {
        *ksh = ((FDHFLAGS(h) & FDHF_KSHLOAD)
                    ? 2
                    : ((FDHFLAGS(h) & FDHF_ZSHLOAD) ? 0 : 1));
      }
      return prog;
    }
  }
  return NULL;
}

/* custom_source and reporting */
/**
 * @brief Execute a sourced script and record a timing event for reporting.
 *
 * Integrates with zsh state (opts, cmdstack, funcstack) and restores on exit.
 * Records duration and paths for later consumption by the report generator.
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
  fd = SHIN;
  osubsh = subsh;
  cj = thisjob;
  oldlineno = lineno;
  oloops = loops;
  oldshst = opts[zp_conv_opt(SHINSTDIN__)]; /* unchanged; explicit suffix */
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
  dosetopt(zp_conv_opt(SHINSTDIN__), 0, 1, opts); /* ensure suffixed enum */
  scriptname = s;
  scriptfilename = s;
  if (isset(zp_conv_opt(SOURCETRACE__))) {
    printprompt4();
    fprintf(xtrerr ? xtrerr : stderr, "<sourcetrace>\n");
  }
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
    switch (loop(0, 0)) {
    case LOOP_OK:
      break;
    case LOOP_EMPTY:
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
  if (prog) {
    freeeprog(prog);
  } else {
    close(SHIN);
    fdtable[SHIN] = FDT_UNUSED;
    SHIN = fd;
    shinbufrestore();
  }
  subsh = osubsh;
  thisjob = cj;
  lineno = oldlineno;
  loops = oloops;
  dosetopt(zp_conv_opt(SHINSTDIN__), oldshst, 1,
           opts); /* ensure suffixed enum */
  errflag &= ~ERRFLAG_ERROR;
  if (!exit_pending) {
    retflag = 0;
  }
  scriptname = old_scriptname;
  scriptfilename = old_scriptfilename;
  zfree(cmdstack, CMDSTACKSZ);
  cmdstack = ocs;
  cmdsp = ocsp;
  zp_tv.tv_sec = zp_tv.tv_usec = 0;
  gettimeofday(&zp_tv, &zp_dummy_tz);
  zp_node = (SEventNode)zshcalloc(sizeof(struct zp_sevent_node));
  if (zp_node) {
    char bkp;
    char *dir_path;
    char *file_name;
    char *full_path;
    char *slash;
    int is_dot_slash;
    if (s[0] == '/') {
      full_path = ztrdup(s);
    } else {
      size_t pwd_len;
      size_t rel_len;
      size_t off;
      is_dot_slash = (s[0] == '.' && s[1] == '/');
      pwd_len = strlen(pwd);
      off = is_dot_slash ? 2U : 0U;
      rel_len = strlen(s) - off;
      full_path = (char *)zalloc(sizeof(char) * (pwd_len + rel_len + 2U));
      int n1 = snprintf(full_path, pwd_len + 1, "%s", pwd);
      (void)n1;
      snprintf(full_path + pwd_len, rel_len + 2U, "/%s", s + off);
    }
    slash = strrchr(full_path, '/');
    file_name = ztrdup(slash + 1);
    bkp = slash[1];
    slash[1] = '\0';
    dir_path = ztrdup(full_path);
    slash[1] = bkp;
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
    char zp_tmp[20];
    snprintf(zp_tmp, sizeof(zp_tmp), "%d", zp_node->event.id);
    zp_tmp[sizeof(zp_tmp) - 1] = '\0';
    if (zp_source_events) {
      addhashnode(zp_source_events, ztrdup(zp_tmp), (void *)zp_node);
    }
  }
  return ret;
}

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
  if (!report) {
    *rep_size = 0;
    return ztrdup("ERROR: couldn't allocate initial buffer, aborted\n");
  }
  space_left = 127;
  report[current_end] = '\0';
  *rep_size = current_size + 1;
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
      current_size += printed - space_left + 25;
      space_left += printed - space_left + 25;
      char *new_report =
          (char *)zrealloc(report, sizeof(char) * (current_size + 1));
      if (!new_report) {
        zfree(report, *rep_size);
        *rep_size = 0;
        return ztrdup("ERROR: Couldn't realloc buffer, aborted\n");
      }
      report = new_report;
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

void zp_free_sevent_node(HashNode hn) {
  SEventNode s = (SEventNode)hn;
  zsfree(hn->nam);
  zsfree(s->event.dir_path);
  zsfree(s->event.file_name);
  zsfree(s->event.full_path);
  zfree(s, sizeof(struct zp_sevent_node));
}

/* Setup/finish helpers for overriding builtin handlers and hashtable */
void zp_source_setup_overrides(void) {
  Builtin bn = (Builtin)builtintab->getnode2(builtintab, ".");
  if (bn) {
    original_dot = bn->handlerfunc;
    bn->handlerfunc = bin_custom_dot;
  }
  bn = (Builtin)builtintab->getnode2(builtintab, "source");
  if (bn) {
    original_source = bn->handlerfunc;
    bn->handlerfunc = bin_custom_dot;
  }
  if (!(zp_source_events = newhashtable(8, "zp_source_events", NULL))) {
    zwarn("Cannot create the hash table");
    return;
  }
  zp_source_events->hash = hasher;
  zp_source_events->emptytable = emptyhashtable;
  zp_source_events->cmpnodes = strcmp;
  zp_source_events->addnode = addhashnode;
  zp_source_events->getnode = gethashnode2;
  zp_source_events->getnode2 = gethashnode2;
  zp_source_events->removenode = removehashnode;
  zp_source_events->freenode = zp_free_sevent_node;
}

void zp_source_restore_overrides(void) {
  Builtin bn = (Builtin)builtintab->getnode2(builtintab, ".");
  if (bn) {
    bn->handlerfunc = original_dot;
  }
  bn = (Builtin)builtintab->getnode2(builtintab, "source");
  if (bn) {
    bn->handlerfunc = original_source;
  }
  if (zp_source_events) {
    deletehashtable(zp_source_events);
    zp_source_events = NULL;
  }
}
