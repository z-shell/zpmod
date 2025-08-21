/* SPDX-License-Identifier: MIT */
#include "zpmod.mdh"
#include "zpmod.pro"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "zpmod_utils.h"
#include "zpmod_fs.h"
#include "zpmod_source.h"
#include "zpmod_emoji.h"

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
    fprintf(stdout, "%s Usage:\n", zp_icon("📘 "));
    fprintf(stdout, "%s zpmod [--help|-h] [--version|-V]\n", zp_icon("🧩 "));
    fprintf(stdout, "%s zpmod report-append <plugin-id> <text>\n", zp_icon("🧩 "));
    fprintf(stdout, "%s zpmod source-study [-l]\n", zp_icon("🧩 "));
    fprintf(stdout, "%s zpmod dirlist [-a] [-d|-f] out_array dir\n", zp_icon("🧩 "));
    fprintf(stdout, "%s zpmod pathstat [-L] [-f fields] out_array in_array\n", zp_icon("🧩 "));
    fprintf(stdout, "%s zpmod readfile [-m] [-d delim|-0] var file\n\n", zp_icon("🧩 "));
    fprintf(stdout, "%s Subcommands:\n", zp_icon("🧰 "));
    fprintf(stdout, "%s report-append   Append <text> to $ZI_REPORTS[<plugin-id>].\n", zp_icon("📝 "));
    fprintf(stdout, "%s source-study    Show sourced files with durations (ms).\n", zp_icon("⏱️ "));
    fprintf(stdout, "%s dirlist         List entries in directory into array.\n", zp_icon("📁 "));
    fprintf(stdout, "%s pathstat        Batch stat for input array into output array.\n", zp_icon("📊 "));
    fprintf(stdout, "%s readfile        Read file into scalar or split into array.\n\n", zp_icon("📄 "));
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

static int cmd_source_study(const char *nam, char **argv) {
  (void)nam;
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
  return 0;
}

static int cmd_dirlist(char *nam, char **argv) {
  int inc_all = 0;
  int only_dirs = 0;
  int only_files = 0;
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
    if (stop) {
      break;
    }
    argv++;
  }
  if (!argv[0] || !argv[1]) {
    zwarnnam(nam, "%sdirlist: usage: zpmod dirlist [-a] [-d|-f] out_array dir", zp_icon("❌ "));
    return 1;
  }
  return zp_dirlist_core(nam, argv[0], argv[1], inc_all, only_dirs, only_files);
}

static int cmd_pathstat(char *nam, char **argv) {
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
    zwarnnam(nam, "%spathstat: usage: zpmod pathstat [-L] [-f fields] out_array in_array", zp_icon("❌ "));
    return 1;
  }
  return zp_pathstat_core(nam, argv[0], argv[1], follow, fields);
}

static int cmd_readfile(char *nam, char **argv) {
  int use_mmap = 0;
  int split = 0;
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
      char *a = NULL; char **cursor = &argv[0];
      int tk = zp_take_opt_with_arg(&cursor, 'd', &a);
      if (tk == -1) { zwarnnam(nam, "%sreadfile: -d requires delimiter", zp_icon("❌ ")); return 1; }
      if (tk == 1) { if (a && *a) { split = 1; delim = parse_delim(a); } argv = cursor; continue; }
    }
    break;
  }
  if (!argv[0] || !argv[1]) {
    zwarnnam(nam, "%sreadfile: usage: zpmod readfile [-m] [-d delim|-0] var file", zp_icon("❌ "));
    return 1;
  }
  return zp_readfile_core(nam, argv[0], argv[1], use_mmap, split, delim);
}

/** `zpmod` builtin entrypoint and subcommand dispatcher. */
int bin_zpmod(char *nam, char **argv, UNUSED(Options ops), UNUSED(int func)) {
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
  } else if (0 == strcmp(subcmd, "dirlist")) {
    ret = cmd_dirlist(nam, argv);
  } else if (0 == strcmp(subcmd, "pathstat")) {
    ret = cmd_pathstat(nam, argv);
  } else if (0 == strcmp(subcmd, "readfile")) {
    ret = cmd_readfile(nam, argv);
  } else {
    zwarnnam(nam, "unknown subcommand: %s. See -h.", subcmd);
  }
  return ret;
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
