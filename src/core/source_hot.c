/* SPDX-License-Identifier: MIT */
/**
 * @file source_hot.c
 * @brief Zcompile hot scripts from source-study data.
 */

#include "zpmod.mdh"
#include "zpmod.pro"
#include "zpmod_vendor_shims.h"
#include "zpmod_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char *path;
    long total_ms;
    int count;
} hot_script_t;

/* Comparator for qsort; parameters mandated by qsort signature. */
static int compare_hot_scripts(const void *a, const void *b) { // NOLINT(bugprone-easily-swappable-parameters)
    hot_script_t *sa = (hot_script_t *)a;
    hot_script_t *sb = (hot_script_t *)b;
    return sb->total_ms - sa->total_ms;
}

/**
 * @brief Implements `zpmod source-hot`.
 *
 * @param nam The builtin name.
 * @param argv Subcommand arguments.
 * @return int 0 on success, 1 on failure.
 */
int cmd_source_hot(char *nam, char **argv) {
    int count = 10;
    int threshold = 0;
    const char *zwc_file = NULL;

    while (*argv && argv[0][0] == '-') {
        if (strcmp(argv[0], "-n") == 0) {
            if (!argv[1]) {
                zwarnnam(nam, "-n requires an argument");
                return 1;
            }
            count = atoi(argv[1]);
            argv += 2;
            continue;
        }
        if (strcmp(argv[0], "--threshold") == 0) {
            if (!argv[1]) {
                zwarnnam(nam, "--threshold requires an argument");
                return 1;
            }
            threshold = atoi(argv[1]);
            argv += 2;
            continue;
        }
        if (strcmp(argv[0], "--zwc") == 0) {
            if (!argv[1]) {
                zwarnnam(nam, "--zwc requires an argument");
                return 1;
            }
            zwc_file = argv[1];
            argv += 2;
            continue;
        }
        if (strcmp(argv[0], "--") == 0) {
            argv++;
            break;
        }
        break;
    }

    /* Feature guard: compile only if source study compiled in */
#ifndef ZPMOD_HAVE_SOURCE_STUDY
    (void)count; (void)threshold; (void)zwc_file; (void)nam; (void)argv;
    return 0;
#endif

    char **history = getaparam("zsh_source_history");
    if (!history) {
        return 0;
    }

    int history_len = arrlen(history);
    hot_script_t *scripts = zalloc(sizeof(hot_script_t) * history_len);
    int script_count = 0;

    for (int i = 0; i < history_len; i++) {
        char *line = history[i];
        int len = (int)strlen(line);
        char *copy = (char *)zalloc(len + 1);
        memcpy(copy, line, len + 1);
        char *saveptr = NULL;
        char *path = strtok_r(copy, " ", &saveptr);
        char *ms_str = strtok_r(NULL, " ", &saveptr);
        if (!path || !ms_str) { continue;
}

        long ms = atol(ms_str);
        if (threshold > 0 && ms < threshold) { continue;
}

        int found = 0;
        for (int j = 0; j < script_count; j++) {
            if (strcmp(scripts[j].path, path) == 0) {
                scripts[j].total_ms += ms;
                scripts[j].count++;
                found = 1;
                break;
            }
        }
        if (!found) {
            scripts[script_count].path = ztrdup(path);
            scripts[script_count].total_ms = ms;
            scripts[script_count].count = 1;
            script_count++;
        }
        zfree(copy, len + 1);
    }

    qsort(scripts, script_count, sizeof(hot_script_t), compare_hot_scripts);

    int limit = (count > 0 && count < script_count) ? count : script_count;

    for (int i = 0; i < limit; i++) {
        char *cmd;
        if (zwc_file) {
            cmd = zalloc(strlen(scripts[i].path) + strlen(zwc_file) + 20);
            sprintf(cmd, "zcompile -o %s %s", zwc_file, scripts[i].path);
        } else {
            cmd = zalloc(strlen(scripts[i].path) + 12);
            sprintf(cmd, "zcompile %s", scripts[i].path);
        }
        execstring(cmd, 1, 0, NULL);
        zsfree(cmd);
    }

    for (int i = 0; i < script_count; i++) {
        zsfree(scripts[i].path);
    }
    zfree(scripts, sizeof(hot_script_t) * history_len);

    return 0;
}
