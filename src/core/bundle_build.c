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
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Portable fallback for platforms that don't define PATH_MAX in <limits.h>. */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Accept these file suffixes (exact) */
static int has_ext(const char *name)
{
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
static void bb_vec_init(struct bb_vec *v)
{
    v->items = NULL;
    v->size = 0;
    v->cap = 0;
}
static int bb_vec_push(struct bb_vec *v, struct bb_entry *e)
{
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
static void bb_vec_free(struct bb_vec *v)
{
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
static int bb_collect(char *nam, const char *root, const char *sub, struct bb_vec *out, const char *output, unsigned depth)
{
    if (depth > 64) {
        errno = ELOOP;
        return 1;
    }
    char path[PATH_MAX];
    int written;
    if (sub && *sub) {
        written = snprintf(path, sizeof(path), "%s/%s", root, sub);
    } else {
        written = snprintf(path, sizeof(path), "%s", root);
    }
    if (written < 0 || written >= (int)sizeof(path)) {
        errno = ENAMETOOLONG;
        return 1;
    }
    DIR *d = opendir(path);
    if (!d) {
        return 1;
    }
    struct dirent *de;
    for (;;) {
        errno = 0;
        de = readdir(d);
        if (!de) {
            break;
        }
        if (de->d_name[0] == '.') {
            continue;
        }
        char rel[PATH_MAX];
        if (sub && *sub) {
            written = snprintf(rel, sizeof(rel), "%s/%s", sub, de->d_name);
        } else {
            written = snprintf(rel, sizeof(rel), "%s", de->d_name);
        }
        if (written < 0 || written >= (int)sizeof(rel) || strchr(rel, '\n') || strchr(rel, '\r')) {
            closedir(d);
            errno = EINVAL;
            return 1;
        }
        char abs[PATH_MAX];
        written = snprintf(abs, sizeof(abs), "%s/%s", root, rel);
        if (written < 0 || written >= (int)sizeof(abs)) {
            closedir(d);
            errno = ENAMETOOLONG;
            return 1;
        }
        if (!strcmp(abs, output)) {
            continue;
        }
        struct stat st;
        if (lstat(abs, &st) != 0) {
            int saved_errno = errno;
            closedir(d);
            errno = saved_errno;
            return 1;
        }
        if (S_ISDIR(st.st_mode)) {
            if (bb_collect(nam, root, rel, out, output, depth + 1)) {
                int saved_errno = errno;
                closedir(d);
                errno = saved_errno;
                return 1;
            }
            errno = 0;
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
    int saved_errno = errno;
    closedir(d);
    errno = saved_errno;
    return saved_errno ? 1 : 0;
}
// NOLINTEND(misc-no-recursion)

/* Lexicographic sort (C locale) */
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int bb_cmp(const void *a, const void *b)
{
    const struct bb_entry *ea = a;
    const struct bb_entry *eb = b;
    return strcmp(ea->rel, eb->rel);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTEND(misc-include-cleaner)

/* Keep publication atomic: failed collection or reads must preserve the old
 * bundle. The temporary file lives beside the requested output. */
int zp_bundle_build_core(char *nam, const char *from_dir, const char *out_path, long max_kb)
{
    if (!from_dir || !*from_dir || !out_path || !*out_path || max_kb < 0 || (unsigned long)max_kb > ULONG_MAX / 1024) {
        zwarnnam(nam, "bundle-build: requires --from and --out and a valid --max");
        return 1;
    }
    size_t from_len = strlen(from_dir) + 1;
    size_t out_len = strlen(out_path) + 1;
    char *from = ztrdup(from_dir);
    char *out = ztrdup(out_path);
    unmetafy(from, NULL);
    unmetafy(out, NULL);
    char root[PATH_MAX];
    char output[PATH_MAX];
    char parent[PATH_MAX];
    int ret = 1;
    struct bb_vec files;
    bb_vec_init(&files);
    FILE *stream = NULL;
    char temporary[PATH_MAX] = "";
    if (!realpath(from, root)) {
        goto done;
    }
    char *slash = strrchr(out, '/');
    const char *base = slash ? slash + 1 : out;
    if (!*base || !strcmp(base, ".") || !strcmp(base, "..")) {
        errno = EINVAL;
        goto done;
    }
    if (slash) {
        *slash = '\0';
    }
    const char *parent_path = ".";
    if (slash) {
        parent_path = *out ? out : "/";
    }
    if (!realpath(parent_path, parent)) {
        goto done;
    }
    int written = snprintf(output, sizeof(output), "%s/%s", parent, base);
    if (written < 0 || written >= (int)sizeof(output)) {
        errno = ENAMETOOLONG;
        goto done;
    }
    struct stat existing;
    int have_output = lstat(output, &existing) == 0;
    if ((have_output && !S_ISREG(existing.st_mode)) || (!have_output && errno != ENOENT)) {
        errno = EINVAL;
        goto done;
    }
    if (bb_collect(nam, root, "", &files, output, 0)) {
        goto done;
    }
    if (files.size > 1) {
        qsort(files.items, files.size, sizeof(*files.items), bb_cmp);
    }
    char header[96];
    snprintf(header, sizeof(header), "# zpmod bundle-build max-kb:%ld\n", max_kb);
    int fresh = have_output;
    for (size_t i = 0; i < files.size; ++i) {
        if (!have_output || files.items[i].mtime > existing.st_mtime) {
            fresh = 0;
        }
    }
    if (fresh) {
        FILE *previous = fopen(output, "r");
        char first_line[96];
        fresh = previous && fgets(first_line, sizeof(first_line), previous) && !strcmp(first_line, header);
        if (previous) {
            fclose(previous);
        }
    }
    if (fresh) {
        ret = 0;
        goto done;
    }
    written = snprintf(temporary, sizeof(temporary), "%s.tmp.XXXXXX", output);
    if (written < 0 || written >= (int)sizeof(temporary)) {
        temporary[0] = '\0';
        errno = ENAMETOOLONG;
        goto done;
    }
    int fd = mkstemp(temporary);
    if (fd < 0) {
        temporary[0] = '\0';
        goto done;
    }
    stream = fdopen(fd, "w");
    if (!stream) {
        close(fd);
        goto done;
    }
    if (fputs(header, stream) == EOF) {
        goto done;
    }
    unsigned long total = 0;
    unsigned long limit = (unsigned long)max_kb * 1024;
    for (size_t i = 0; i < files.size; ++i) {
        struct bb_entry *entry = &files.items[i];
        if (entry->size < 0 || (unsigned long long)entry->size > ULONG_MAX - total) {
            errno = EOVERFLOW;
            goto done;
        }
        if (limit && (unsigned long)entry->size > limit - total) {
            zwarnnam(nam, "bundle-build: truncated at --max %L KiB", max_kb);
            break;
        }
#ifdef O_NOFOLLOW
        int input = open(entry->abs, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
#else
        errno = ENOTSUP;
        int input = -1;
#endif
        if (input < 0) {
            goto done;
        }
        struct stat st;
        if (fstat(input, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size != entry->size || st.st_mtime != entry->mtime) {
            close(input);
            errno = EAGAIN;
            goto done;
        }
        FILE *source = fdopen(input, "r");
        if (!source) {
            close(input);
            goto done;
        }
        int failed = fprintf(stream, "# BEGIN %s\n", entry->rel) < 0;
        char buffer[8192];
        size_t bytes;
        unsigned long copied = 0;
        while (!failed && !feof(source) && !ferror(source)) {
            bytes = fread(buffer, 1, sizeof(buffer), source);
            if (!bytes) {
                break;
            }
            if (bytes > (unsigned long)entry->size - copied || fwrite(buffer, 1, bytes, stream) != bytes) {
                failed = 1;
                break;
            }
            copied += bytes;
        }
        failed |= ferror(source) || copied != (unsigned long)entry->size;
        failed |= fclose(source) != 0;
        if (failed || fprintf(stream, "\n# END %s\n", entry->rel) < 0) {
            errno = EIO;
            goto done;
        }
        total += copied;
    }
    if (fclose(stream) != 0) {
        stream = NULL;
        goto done;
    }
    stream = NULL;
    if (rename(temporary, output) != 0) {
        goto done;
    }
    temporary[0] = '\0';
    ret = 0;
done:
    if (ret) {
        zwarnnam(nam, "bundle-build: %s", strerror(errno));
    }
    if (stream) {
        fclose(stream);
    }
    if (*temporary) {
        unlink(temporary);
    }
    bb_vec_free(&files);
    zfree(from, from_len);
    zfree(out, out_len);
    return ret;
}
