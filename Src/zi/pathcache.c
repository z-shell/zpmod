#include "pathcache.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

/**
 * Simple string hash function
 */
static unsigned int zp_hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**
 * Initialize the path cache
 */
ZpPathCache zp_path_cache_init(int size, time_t lifetime) {
    ZpPathCache cache = (ZpPathCache)malloc(sizeof(struct zp_path_cache));
    if (!cache)
        return NULL;

    cache->size = size;
    cache->count = 0;
    cache->cache_lifetime = lifetime;

    cache->buckets = (ZpPathCacheEntry*)calloc(size, sizeof(ZpPathCacheEntry));
    if (!cache->buckets) {
        free(cache);
        return NULL;
    }

    return cache;
}

/**
 * Free all resources used by the path cache
 */
void zp_path_cache_destroy(ZpPathCache cache) {
    if (!cache)
        return;

    zp_path_cache_clear(cache);
    free(cache->buckets);
    free(cache);
}

/**
 * Create a new cache entry
 */
static ZpPathCacheEntry zp_create_cache_entry(const char *path, struct stat *buf, int exists, int is_regular) {
    ZpPathCacheEntry entry = (ZpPathCacheEntry)malloc(sizeof(struct zp_path_cache_entry));
    if (!entry)
        return NULL;

    entry->path = strdup(path);
    if (!entry->path) {
        free(entry);
        return NULL;
    }

    entry->cache_time = time(NULL);
    entry->exists = exists;
    entry->is_regular = is_regular;
    entry->next = NULL;

    if (exists && buf)
        memcpy(&entry->stat_info, buf, sizeof(struct stat));

    return entry;
}

/**
 * Free a cache entry
 */
static void zp_free_cache_entry(ZpPathCacheEntry entry) {
    if (!entry)
        return;

    free(entry->path);
    free(entry);
}

/**
 * Find a cache entry for a path
 */
static ZpPathCacheEntry zp_find_cache_entry(ZpPathCache cache, const char *path) {
    if (!cache || !path)
        return NULL;

    unsigned int hash = zp_hash_string(path) % cache->size;
    ZpPathCacheEntry entry = cache->buckets[hash];
    time_t now = time(NULL);

    while (entry) {
        if (strcmp(entry->path, path) == 0) {
            /* Check if entry is still valid */
            if (now - entry->cache_time <= cache->cache_lifetime)
                return entry;
            else
                return NULL; /* Entry expired */
        }
        entry = entry->next;
    }

    return NULL;
}

/**
 * Add a new cache entry
 */
static ZpPathCacheEntry zp_add_cache_entry(ZpPathCache cache, const char *path,
                                          struct stat *buf, int exists, int is_regular) {
    if (!cache || !path)
        return NULL;

    unsigned int hash = zp_hash_string(path) % cache->size;
    ZpPathCacheEntry entry = zp_create_cache_entry(path, buf, exists, is_regular);

    if (!entry)
        return NULL;

    /* Add to front of bucket */
    entry->next = cache->buckets[hash];
    cache->buckets[hash] = entry;
    cache->count++;

    return entry;
}

/**
 * Get stat information for a path, using cache when possible
 */
int zp_path_cache_stat(ZpPathCache cache, const char *path, struct stat *buf) {
    if (!cache || !path || !buf)
        return 1;

    /* Check cache first */
    ZpPathCacheEntry entry = zp_find_cache_entry(cache, path);
    if (entry) {
        if (entry->exists) {
            memcpy(buf, &entry->stat_info, sizeof(struct stat));
            return 0;
        } else {
            return 1;
        }
    }

    /* Not in cache, do actual stat and cache result */
    struct stat local_stat;
    int result = stat(path, &local_stat);

    if (result == 0) {
        memcpy(buf, &local_stat, sizeof(struct stat));
        zp_add_cache_entry(cache, path, &local_stat, 1, S_ISREG(local_stat.st_mode));
    } else {
        zp_add_cache_entry(cache, path, NULL, 0, 0);
    }

    return result;
}

/**
 * Check if a path exists in the filesystem, using cache when possible
 */
int zp_path_cache_exists(ZpPathCache cache, const char *path) {
    if (!cache || !path)
        return 0;

    /* Check cache first */
    ZpPathCacheEntry entry = zp_find_cache_entry(cache, path);
    if (entry)
        return entry->exists;

    /* Not in cache, do actual check and cache result */
    struct stat buf;
    int exists = (access(path, F_OK) == 0);

    if (exists) {
        if (stat(path, &buf) == 0) {
            zp_add_cache_entry(cache, path, &buf, 1, S_ISREG(buf.st_mode));
        } else {
            exists = 0;
            zp_add_cache_entry(cache, path, NULL, 0, 0);
        }
    } else {
        zp_add_cache_entry(cache, path, NULL, 0, 0);
    }

    return exists;
}

/**
 * Check if a path is a regular file, using cache when possible
 */
int zp_path_cache_is_regular(ZpPathCache cache, const char *path) {
    if (!cache || !path)
        return 0;

    /* Check cache first */
    ZpPathCacheEntry entry = zp_find_cache_entry(cache, path);
    if (entry)
        return entry->is_regular;

    /* Not in cache, do actual check and cache result */
    struct stat buf;
    int is_regular = 0;

    if (stat(path, &buf) == 0) {
        is_regular = S_ISREG(buf.st_mode);
        zp_add_cache_entry(cache, path, &buf, 1, is_regular);
    } else {
        zp_add_cache_entry(cache, path, NULL, 0, 0);
    }

    return is_regular;
}

/**
 * Invalidate a specific path in the cache
 */
void zp_path_cache_invalidate(ZpPathCache cache, const char *path) {
    if (!cache || !path)
        return;

    unsigned int hash = zp_hash_string(path) % cache->size;
    ZpPathCacheEntry entry = cache->buckets[hash];
    ZpPathCacheEntry prev = NULL;

    while (entry) {
        if (strcmp(entry->path, path) == 0) {
            if (prev)
                prev->next = entry->next;
            else
                cache->buckets[hash] = entry->next;

            zp_free_cache_entry(entry);
            cache->count--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

/**
 * Clear all entries from the cache
 */
void zp_path_cache_clear(ZpPathCache cache) {
    if (!cache)
        return;

    for (int i = 0; i < cache->size; i++) {
        ZpPathCacheEntry entry = cache->buckets[i];
        while (entry) {
            ZpPathCacheEntry next = entry->next;
            zp_free_cache_entry(entry);
            entry = next;
        }
        cache->buckets[i] = NULL;
    }

    cache->count = 0;
}
