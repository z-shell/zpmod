#ifndef ZPMOD_COMPILE_CONFIG_H
#define ZPMOD_COMPILE_CONFIG_H

#include <sys/types.h>
#include <regex.h>
#include <time.h>

/**
 * Structure to hold compilation configuration settings
 */
typedef struct zp_compile_config {
    int enabled;                 /* Whether automatic compilation is enabled */
    int debug_mode;              /* Debug mode flag */
    int batch_mode;              /* Whether to use batch compilation */
    int batch_size;              /* Maximum number of files in a batch */
    int batch_interval;          /* Interval between batch compilation jobs in seconds */
    int max_file_size;           /* Maximum file size to compile in bytes */
    char **exclusion_patterns;   /* Array of glob patterns to exclude from compilation */
    int exclusion_count;         /* Number of exclusion patterns */
    regex_t *exclusion_regex;    /* Compiled regex patterns */
    char **inclusion_patterns;   /* Array of glob patterns to always compile */
    int inclusion_count;         /* Number of inclusion patterns */
    regex_t *inclusion_regex;    /* Compiled regex patterns */
    time_t last_batch_time;      /* Time of last batch compilation */
    char **pending_files;        /* Files waiting for batch compilation */
    int pending_count;           /* Number of pending files */
    int pending_alloc;           /* Allocated size for pending_files */
} *ZpCompileConfig;

/**
 * Initialize compilation configuration with default settings
 *
 * @return Pointer to initialized configuration or NULL on failure
 */
ZpCompileConfig zp_compile_config_init(void);

/**
 * Free all resources used by the compilation configuration
 *
 * @param config The configuration to free
 */
void zp_compile_config_destroy(ZpCompileConfig config);

/**
 * Load configuration from environment variables
 *
 * @param config The configuration to update
 */
void zp_compile_config_load_env(ZpCompileConfig config);

/**
 * Add an exclusion pattern to the configuration
 *
 * @param config The configuration to update
 * @param pattern The glob pattern to exclude from compilation
 * @return 0 on success, non-zero on failure
 */
int zp_compile_config_add_exclusion(ZpCompileConfig config, const char *pattern);

/**
 * Add an inclusion pattern to the configuration
 *
 * @param config The configuration to update
 * @param pattern The glob pattern to always compile
 * @return 0 on success, non-zero on failure
 */
int zp_compile_config_add_inclusion(ZpCompileConfig config, const char *pattern);

/**
 * Check if a file should be excluded from compilation
 *
 * @param config The configuration to use
 * @param file The file to check
 * @return 1 if the file should be excluded, 0 otherwise
 */
int zp_compile_config_should_exclude(ZpCompileConfig config, const char *file);

/**
 * Check if a file should be included for compilation (override exclusions)
 *
 * @param config The configuration to use
 * @param file The file to check
 * @return 1 if the file should be included, 0 otherwise
 */
int zp_compile_config_should_include(ZpCompileConfig config, const char *file);

/**
 * Add a file to the pending compilation batch
 *
 * @param config The configuration to use
 * @param file The file to add
 * @return 0 on success, non-zero on failure
 */
int zp_compile_config_add_pending(ZpCompileConfig config, const char *file);

/**
 * Process pending compilation batch if conditions are met
 *
 * @param config The configuration to use
 * @return 0 on success, non-zero on failure
 */
int zp_compile_config_process_batch(ZpCompileConfig config);

/**
 * Clear all pending files from the batch
 *
 * @param config The configuration to use
 */
void zp_compile_config_clear_pending(ZpCompileConfig config);

#endif /* ZPMOD_COMPILE_CONFIG_H */
