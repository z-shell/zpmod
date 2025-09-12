/* SPDX-License-Identifier: MIT */
/* Canonical module header ordering */
#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_vendor_shims.h"
/* System headers after gateway */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "zpmod_utils.h"
#include "zpmod_fs.h"
#include "zpmod_source.h"
#include "zpmod_emoji.h"
#include "zpmod_compaudit.h"
#include "zpmod_rehash.h"
#include "zpmod_bundle.h"

/* Parse escaped delimiter specifications like \n, \t, \0, \r */
static int parse_delim(const char *a) {
  if (!a || !*a) { return '\n';
}
  if (a[0] == '\\') {
    switch (a[1]) {
    case 'n': return '\n';
    case 't': return '\t';
    case '0': return '\0';
    case 'r': return '\r';
    default:  return (unsigned char)a[1];
    }
  }
  return (unsigned char)a[0];
}

/*
 * Append text to $ZI_REPORTS[<plugin>].
 * Notes:
 * - Uses zsh memory allocators (zalloc/zsfree)
 * - Emits user-facing diagnostics via zwarnnam (emoji inside function)
 */
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int zp_append_report(const char *nam /* reporting name */, const char *target,
                            const char *body, int body_len) {
  if (!body_len) { return 0; /* noop */
}

  /* Lookup associative array parameter */
  Param pm = (Param)paramtab->getnode(paramtab, "ZI_REPORTS");
  if (!pm) {
    zwarnnam(nam, "%s$ZI_REPORTS is not declared (zpmod not loaded?).", zp_icon("❌ "));
    return 1;
  }
  HashTable ht = pm->u.hash;
  HashNode hn = gethashnode2(ht, target);
  Param val_pm = (Param)hn;
  if (!val_pm) {
    zwarnnam(nam, "%sunknown plugin: %s", zp_icon("❌ "), target);
    return 1;
  }

  char *target_string = val_pm->u.str;
  const int target_len = target_string ? (int)strlen(target_string) : 0;
  const int new_len = target_len + body_len;

  char *newbuf = (char *)zalloc((new_len + 1) * sizeof(char));
  if (!newbuf) {
    zwarnnam(nam, "%sout of memory", zp_icon("❌ "));
    return 1;
  }
  if (target_len) { memcpy(newbuf, target_string, (size_t)target_len);
}
  if (body_len) { memcpy(newbuf + target_len, body, (size_t)body_len);
}
  newbuf[new_len] = '\0';

  if (val_pm->u.str) { zsfree(val_pm->u.str);
}
  val_pm->u.str = newbuf;
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

/** Print usage for the `zpmod` builtin. */
void zpmod_usage(void) {
  /* Section headings use emojis; lines use a consistent indent and columns for alignment. */
  fprintf(stdout, "%s Usage:\n", zp_icon("📘 "));
  fprintf(stdout, "  zpmod [--help|-h] [--version|-V]\n");
  fprintf(stdout, "  zpmod report-append <plugin-id> <text>\n");
  fprintf(stdout, "  zpmod source-study [-l]\n");
  fprintf(stdout, "  zpmod dir-list [-a] [-d|-f] out_array dir\n");
  fprintf(stdout, "  zpmod path-stat [-L] [-f fields] out_array in_array\n");
  fprintf(stdout, "  zpmod path-warmup [-q] [--prune-missing] [--dry-run]\n");
  fprintf(stdout, "  zpmod compaudit-cache [--rebuild] [--show] [--json]\n");
  fprintf(stdout, "  zpmod rehash-diff\n");
  fprintf(stdout, "  zpmod bundle-build --from DIR --out FILE [--max KB]\n");
  fprintf(stdout, "  zpmod read-file [-m] [-d delim|-0] var file\n\n");

  fprintf(stdout, "%s Subcommands:\n", zp_icon("🧰 "));
  /* Fixed-width label column for readability (max subcommand length ~12). */
  fprintf(stdout, "  %-14s %s\n", "report-append", "Append <text> to $ZI_REPORTS[<plugin-id>].");
  fprintf(stdout, "  %-14s %s\n", "source-study",  "Show sourced files with durations (ms).");
  fprintf(stdout, "  %-14s %s\n", "dir-list",        "List entries in directory into array.");
  fprintf(stdout, "  %-14s %s\n", "path-stat",       "Batch stat for input array into output array.");
  fprintf(stdout, "  %-14s %s\n", "path-warmup",    "Touch PATH dirs to warm kernel VFS caches.");
  fprintf(stdout, "  %-14s %s\n\n", "read-file",       "Read file into scalar or split into array.");

  fprintf(stdout, "%s Options:\n", zp_icon("⚙️ "));
  fflush(stdout);
}

/* Subcommand helpers to keep bin_zpmod simple */
static int cmd_report_append(char *nam, char **argv) {
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
    zfree(target, target_len);
    return 1;
  }
  body_len = (int)strlen(body);
  {
    int rc = zp_append_report(nam, target, body, body_len);
    zfree(target, target_len);
    return rc;
  }
}

int bin_zpmod(char *nam, char **argv, Options ops, int func) { (void)func; /* unused */
  char *subcmd = NULL;
  int ret = 0;
  if (OPT_ISSET(ops, 'V') ||
      (argv && argv[0] &&
       (!strcmp(argv[0], "--version") || !strcmp(argv[0], "-V")))) {
    fprintf(stdout, "%szpmod %s (git: %s)\n", zp_icon("🧩 "), ZPMOD_VERSION_STR,
            ZPMOD_GIT_DESCRIBE_STR);
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
    ret = cmd_report_append(nam, argv);
  } else if (0 == strcmp(subcmd, "source-study")) {
    ret = cmd_source_study(nam, argv);
  } else if (0 == strcmp(subcmd, "source-hot")) {
    ret = cmd_source_hot(nam, argv);
  } else if (0 == strcmp(subcmd, "path-warmup")) {
    int quiet = 0; int prune_missing = 0; int dry_run = 0;
    while (*argv && argv[0][0] == '-') {
      if (strcmp(argv[0], "-q") == 0 || strcmp(argv[0], "--quiet") == 0) { quiet = 1; argv++; continue; }
      if (strcmp(argv[0], "--prune-missing") == 0) { prune_missing = 1; argv++; continue; }
      if (strcmp(argv[0], "--dry-run") == 0) { dry_run = 1; argv++; continue; }
      if (strcmp(argv[0], "--") == 0) { argv++; break; }
      break;
    }
    (void)argv; /* no trailing args */
    int n = zp_path_warmup_core(nam, quiet, prune_missing, dry_run);
    (void)n; /* could print if not quiet */
    ret = 0;
  } else if (0 == strcmp(subcmd, "fpath-index")) {
    ret = cmd_fpath_index(nam, argv);
  } else if (0 == strcmp(subcmd, "compaudit-cache")) {
    int rebuild = 0; int show = 0; int json = 0;
    while (*argv && argv[0][0] == '-') {
      if (!strcmp(argv[0], "--rebuild")) { rebuild = 1; argv++; continue; }
      if (!strcmp(argv[0], "--show")) { show = 1; argv++; continue; }
      if (!strcmp(argv[0], "--json")) { json = 1; argv++; continue; }
      if (!strcmp(argv[0], "--")) { argv++; break; }
      break;
    }
    ret = zp_compaudit_cache_core(nam, rebuild, show, json);
  } else if (0 == strcmp(subcmd, "rehash-diff")) {
    ret = zp_rehash_diff_core(nam);
  } else if (0 == strcmp(subcmd, "bundle-build")) {
    const char *from_dir = NULL; const char *out_path = NULL; long max_kb = 0;
    while (*argv) {
      if (!strcmp(argv[0], "--from")) { if (argv[1]) { from_dir = argv[1]; argv += 2; continue; } zwarnnam(nam, "bundle-build: --from requires value"); ret = 1; break; }
      if (!strcmp(argv[0], "--out")) { if (argv[1]) { out_path = argv[1]; argv += 2; continue; } zwarnnam(nam, "bundle-build: --out requires value"); ret = 1; break; }
      if (!strcmp(argv[0], "--max")) { if (argv[1]) { char *end=NULL; long v=strtol(argv[1], &end, 10); if (*end=='\0' && v>=0) { max_kb = v; } else { zwarnnam(nam, "bundle-build: invalid --max value: %s", argv[1]); ret = 1; } argv += 2; continue; } zwarnnam(nam, "bundle-build: --max requires value"); ret = 1; break; }
      if (!strcmp(argv[0], "--")) { argv++; break; }
      break;
    }
    if (ret == 0) { ret = zp_bundle_build_core(nam, from_dir, out_path, max_kb);
}
  } else if (0 == strcmp(subcmd, "dir-list")) {
    ret = cmd_dirlist(nam, argv);
  } else if (0 == strcmp(subcmd, "path-stat")) {
    ret = cmd_pathstat(nam, argv);
  } else if (0 == strcmp(subcmd, "read-file")) {
    ret = cmd_readfile(nam, argv);
  } else {
    zwarnnam(nam, "unknown subcommand: %s. See -h.", subcmd);
  }
  return ret;
}

int cmd_source_study(char *nam, char **argv) {
  int report_count = 10;
  int threshold_ms = 0;
  int clear_history = 0;
  while (*argv) {
    if (strcmp(argv[0], "--") == 0) {
      argv++;
      break;
    }
    if (strcmp(argv[0], "-l") == 0) {
      clear_history = 1;
      argv++;
      continue;
    }
    if (argv[0][0] == '-') {
      zwarnnam(nam, "source-study: unknown option: %s", argv[0]);
      return 1;
    }
    char *endptr = NULL;
    long val = strtol(argv[0], &endptr, 10);
    if (*endptr == '\0' && val >= 0 && val <= INT_MAX) {
      report_count = (int)val;
      argv++;
    } else {
      zwarnnam(nam, "source-study: invalid report count: %s", argv[0]);
      return 1;
    }
  }
  if (!zp_source_study_core) {
    /* Weak symbol not present: treat as gracefully unavailable */
    return 0; /* success (no data) */
  }
  return zp_source_study_core(nam, report_count, threshold_ms, clear_history);
}

int cmd_dirlist(char *nam, char **argv) {
  int inc_all = 0;
  int only_dirs = 0;
  int only_files = 0;
  while (*argv && argv[0][0] == '-') {
    if (!strcmp(argv[0], "-a")) { inc_all = 1; argv++; continue; }
    if (!strcmp(argv[0], "-d")) { only_dirs = 1; argv++; continue; }
    if (!strcmp(argv[0], "-f")) { only_files = 1; argv++; continue; }
    if (!strcmp(argv[0], "--")) { argv++; break; }
    break;
  }
  if (!argv[0] || !argv[1]) {
    zwarnnam(nam, "dir-list: usage: zpmod dir-list [-a] [-d|-f] out_array dir");
    return 1;
  }
  return zp_dirlist_core(nam, argv[0], argv[1], inc_all, only_dirs, only_files);
}

int cmd_pathstat(char *nam, char **argv) {
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
      char **cursor = &argv[0];
      int tk = zp_take_opt_with_arg(&cursor, 'f', &fields);
      if (tk == -1) { zwarnnam(nam, "%spathstat: -f requires fields", zp_icon("❌ ")); return 1; }
      if (tk == 1) { argv = cursor; continue; }
    }
    break;
  }
  if (!argv[0] || !argv[1]) {
    zwarnnam(nam, "%spathstat: usage: zpmod path-stat [-L] [-f fields] out_array in_array", zp_icon("❌ "));
    return 1;
  }
  return zp_pathstat_core(nam, argv[0], argv[1], follow, fields);
}

int cmd_readfile(char *nam, char **argv) {
  int use_mmap = 0;
  int split = 0;
  int delim = '\n';
  while (*argv && argv[0][0] == '-') {
    if (strcmp(argv[0], "--mmap") == 0) { use_mmap = 1; argv++; continue; }
    if (strcmp(argv[0], "-0") == 0) { split = 1; delim = '\0'; argv++; continue; }
    if (strcmp(argv[0], "--") == 0) { argv++; break; }
    if (argv[0][1] == 'd') {
      /* Allow forms: -d X  or -dX */
      if (argv[0][2]) {
        const char *a = argv[0] + 2;
        split = 1;
        if (a[0] == '\\' && a[1]) {
          switch (a[1]) { case 'n': delim='\n'; break; case 't': delim='\t'; break; case '0': delim='\0'; break; case 'r': delim='\r'; break; default: delim=(unsigned char)a[1]; break; }
        } else { delim = (unsigned char)a[0]; }
        argv++;
        continue;
      } if (argv[1]) {
        const char *a = argv[1];
        split = 1;
        if (a[0] == '\\') {
          if (a[1]) {
            switch (a[1]) { case 'n': delim='\n'; break; case 't': delim='\t'; break; case '0': delim='\0'; break; case 'r': delim='\r'; break; default: delim=(unsigned char)a[1]; break; }
          } else {
            delim = '\\';
          }
        } else { delim = (unsigned char)a[0]; }
        argv += 2; /* consume -d and arg */
        continue;
      }         zwarnnam(nam, "read-file: -d requires delimiter");
        return 1;

    }
    break; /* unknown option */
  }
  if (!argv[0] || !argv[1]) {
    zwarnnam(nam, "read-file: usage: zpmod read-file [-m|--mmap] [-0|-d delim] var file");
    return 1;
  }
  return zp_readfile_core(nam, argv[0], argv[1], use_mmap, split, delim);
}

/* Expose builtins table from this TU */
static struct builtin self_builtins[] = {
    BUILTIN("zpmod", 0, bin_zpmod, 0, -1, 0, "hV", NULL),
};

struct builtin *zp_get_self_builtins(size_t *count) {
  if (count) {
    *count = sizeof(self_builtins) / sizeof(*self_builtins);
  }
  return self_builtins;
}
