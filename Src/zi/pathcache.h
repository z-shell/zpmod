#ifndef ZPMOD_PATHCACHE_H
#define ZPMOD_PATHCACHE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

/**
 * Path cache entry structure
 */
typedef struct zp_path_cache_entry {
    char *path;                  /* Cached file path */
    struct stat stat_info;       /* Cached stat information */
    time_t cache_time;           /* Time when this entry was cached */
    int exists;                  /* Whether the file exists */
    int is_regular;              /* Whether it's a regular file */
    struct zp_path_cache_entry *next;  /* Next entry in hash bucket */
} *ZpPathCacheEntry;

/**
 * Path cache hash table structure
 */
typedef struct zp_path_cache {
    int size;                    /* Hash table size */
    int count;                   /* Number of entries */
    time_t cache_lifetime;       /* How long entries remain valid (seconds) */
    ZpPathCacheEntry *buckets;   /* Hash buckets */
} *ZpPathCache;

/**
 * Initialize the path cache
 *
 * @param size Hash table size
 * @param lifetime Cache entry lifetime in seconds
 * @return Pointer to initialized cache or NULL on failure
 */
ZpPathCache zp_path_cache_init(int size, time_t lifetime);

/**
 * Free all resources used by the path cache
 *
 * @param cache The cache to free
 */
void zp_path_cache_destroy(ZpPathCache cache);

/**
 * Get stat information for a path, using cache when possible
 *
 * @param cache The path cache
 * @param path The path to stat
 * @param buf Where to store stat information
 * @return 0 on success, 1 on failure (like stat)
 */
int zp_path_cache_stat(ZpPathCache cache, const char *path, struct stat *buf);

/**
 * Check if a path exists in the filesystem, using cache when possible
 *
 * @param cache The path cache
 * @param path The path to check
 * @return 1 if exists, 0 if not
 */
int zp_path_cache_exists(ZpPathCache cache, const char *path);

/**
 * Check if a path is a regular file, using cache when possible
 *
 * @param cache The path cache
 * @param path The path to check
 * @return 1 if it's a regular file, 0 if not
 */
int zp_path_cache_is_regular(ZpPathCache cache, const char *path);

/**
 * Invalidate a specific path in the cache
 *
 * @param cache The path cache
 * @param path The path to invalidate
 */
void zp_path_cache_invalidate(ZpPathCache cache, const char *path);

/**
 * Clear all entries from the cache
 *
 * @param cache The path cache to clear
 */
void zp_path_cache_clear(ZpPathCache cache);

#endif /* ZPMOD_PATHCACHE_H */
