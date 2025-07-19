#include "compileconfig.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <stdio.h>

/* For environment variables */
extern char **environ;

/* Default compilation settings */
#define DEFAULT_ENABLED 1
#define DEFAULT_DEBUG_MODE 0
#define DEFAULT_BATCH_MODE 1
#define DEFAULT_BATCH_SIZE 10
#define DEFAULT_BATCH_INTERVAL 60  /* 60 seconds */
#define DEFAULT_MAX_FILE_SIZE (1024 * 1024)  /* 1MB */

/* Environment variables */
#define ENV_ZI_COMPILE_ENABLED "ZI_COMPILE_ENABLED"
#define ENV_ZI_COMPILE_DEBUG "ZI_MOD_DEBUG"
#define ENV_ZI_COMPILE_BATCH "ZI_COMPILE_BATCH"
#define ENV_ZI_COMPILE_BATCH_SIZE "ZI_COMPILE_BATCH_SIZE"
#define ENV_ZI_COMPILE_BATCH_INTERVAL "ZI_COMPILE_BATCH_INTERVAL"
#define ENV_ZI_COMPILE_MAX_SIZE "ZI_COMPILE_MAX_SIZE"
#define ENV_ZI_COMPILE_EXCLUDE "ZI_COMPILE_EXCLUDE"
#define ENV_ZI_COMPILE_INCLUDE "ZI_COMPILE_INCLUDE"

/* Default exclusion patterns */
static const char *default_exclusions[] = {
    "*.zwc",                   /* Already compiled */
    "*.zwc.old",               /* Old compiled files */
    "*.md",                    /* Markdown files */
    "*.txt",                   /* Text files */
    "*.jpg", "*.jpeg", "*.png", "*.gif", /* Images */
    "*.pdf", "*.doc*", "*.xls*", "*.ppt*", /* Documents */
    "*.tar", "*.gz", "*.zip", "*.rar", /* Archives */
    "*/cache/*",               /* Cache directories */
    "*/tmp/*",                 /* Temporary directories */
    "*/.git/*",                /* Git directories */
    "*/.svn/*",                /* SVN directories */
    "*/.hg/*",                 /* Mercurial directories */
    "*/.bzr/*",                /* Bazaar directories */
    "*/node_modules/*",        /* Node.js modules */
    "*/vendor/*",              /* Vendor directories */
    NULL
};

/**
 * Initialize compilation configuration with default settings
 */
ZpCompileConfig zp_compile_config_init(void)
{
    ZpCompileConfig config = (ZpCompileConfig)malloc(sizeof(struct zp_compile_config));
    if (!config)
        return NULL;

    /* Set default values */
    config->enabled = DEFAULT_ENABLED;
    config->debug_mode = DEFAULT_DEBUG_MODE;
    config->batch_mode = DEFAULT_BATCH_MODE;
    config->batch_size = DEFAULT_BATCH_SIZE;
    config->batch_interval = DEFAULT_BATCH_INTERVAL;
    config->max_file_size = DEFAULT_MAX_FILE_SIZE;
    config->exclusion_patterns = NULL;
    config->exclusion_count = 0;
    config->exclusion_regex = NULL;
    config->inclusion_patterns = NULL;
    config->inclusion_count = 0;
    config->inclusion_regex = NULL;
    config->last_batch_time = time(NULL);
    config->pending_files = NULL;
    config->pending_count = 0;
    config->pending_alloc = 0;

    /* Add default exclusion patterns */
    for (int i = 0; default_exclusions[i] != NULL; i++) {
        zp_compile_config_add_exclusion(config, default_exclusions[i]);
    }

    return config;
}

/**
 * Free all resources used by the compilation configuration
 */
void zp_compile_config_destroy(ZpCompileConfig config)
{
    if (!config)
        return;

    /* Free exclusion patterns */
    if (config->exclusion_patterns) {
        for (int i = 0; i < config->exclusion_count; i++) {
            free(config->exclusion_patterns[i]);
        }
        free(config->exclusion_patterns);
    }

    /* Free compiled regex patterns */
    if (config->exclusion_regex) {
        for (int i = 0; i < config->exclusion_count; i++) {
            regfree(&config->exclusion_regex[i]);
        }
        free(config->exclusion_regex);
    }

    /* Free inclusion patterns */
    if (config->inclusion_patterns) {
        for (int i = 0; i < config->inclusion_count; i++) {
            free(config->inclusion_patterns[i]);
        }
        free(config->inclusion_patterns);
    }

    /* Free compiled inclusion regex patterns */
    if (config->inclusion_regex) {
        for (int i = 0; i < config->inclusion_count; i++) {
            regfree(&config->inclusion_regex[i]);
        }
        free(config->inclusion_regex);
    }

    /* Free pending files */
    if (config->pending_files) {
        for (int i = 0; i < config->pending_count; i++) {
            free(config->pending_files[i]);
        }
        free(config->pending_files);
    }

    free(config);
}

/**
 * Helper function to get environment variable as integer
 */
static int get_env_int(const char *name, int default_value)
{
    char *value = getenv(name);
    if (!value || !*value)
        return default_value;

    char *endptr;
    long result = strtol(value, &endptr, 10);

    if (*endptr != '\0' || result < 0)
        return default_value;

    return (int)result;
}

/**
 * Helper function to get environment variable as string list
 * Format: colon-separated list of patterns
 */
static char **get_env_list(const char *name, int *count)
{
    char *value = getenv(name);
    if (!value || !*value) {
        *count = 0;
        return NULL;
    }

    /* Count the number of patterns */
    int pattern_count = 1;
    for (char *p = value; *p; p++) {
        if (*p == ':')
            pattern_count++;
    }

    /* Allocate array for patterns */
    char **patterns = (char **)malloc(pattern_count * sizeof(char *));
    if (!patterns) {
        *count = 0;
        return NULL;
    }

    /* Split the value into patterns */
    char *copy = strdup(value);
    char *token = strtok(copy, ":");
    int i = 0;

    while (token && i < pattern_count) {
        patterns[i++] = strdup(token);
        token = strtok(NULL, ":");
    }

    free(copy);
    *count = i;
    return patterns;
}

/**
 * Load configuration from environment variables
 */
void zp_compile_config_load_env(ZpCompileConfig config)
{
    if (!config)
        return;

    /* Get enabled flag */
    config->enabled = get_env_int(ENV_ZI_COMPILE_ENABLED, DEFAULT_ENABLED);

    /* Get debug mode */
    char *debug_value = getenv(ENV_ZI_COMPILE_DEBUG);
    config->debug_mode = (debug_value && 0 == strcmp(debug_value, "1"));

    /* Get batch mode */
    config->batch_mode = get_env_int(ENV_ZI_COMPILE_BATCH, DEFAULT_BATCH_MODE);

    /* Get batch size */
    config->batch_size = get_env_int(ENV_ZI_COMPILE_BATCH_SIZE, DEFAULT_BATCH_SIZE);

    /* Get batch interval */
    config->batch_interval = get_env_int(ENV_ZI_COMPILE_BATCH_INTERVAL, DEFAULT_BATCH_INTERVAL);

    /* Get max file size */
    config->max_file_size = get_env_int(ENV_ZI_COMPILE_MAX_SIZE, DEFAULT_MAX_FILE_SIZE);

    /* Get exclusion patterns */
    char **exclusions;
    int exclusion_count;

    exclusions = get_env_list(ENV_ZI_COMPILE_EXCLUDE, &exclusion_count);
    if (exclusions) {
        /* Add each exclusion pattern */
        for (int i = 0; i < exclusion_count; i++) {
            zp_compile_config_add_exclusion(config, exclusions[i]);
            free(exclusions[i]);
        }
        free(exclusions);
    }

    /* Get inclusion patterns */
    char **inclusions;
    int inclusion_count;

    inclusions = get_env_list(ENV_ZI_COMPILE_INCLUDE, &inclusion_count);
    if (inclusions) {
        /* Add each inclusion pattern */
        for (int i = 0; i < inclusion_count; i++) {
            zp_compile_config_add_inclusion(config, inclusions[i]);
            free(inclusions[i]);
        }
        free(inclusions);
    }
}

/**
 * Convert glob pattern to regex pattern
 */
static char *glob_to_regex(const char *glob)
{
    /* Worst-case allocation: each character becomes two plus anchors and null */
    char *regex = (char *)malloc(strlen(glob) * 2 + 3);
    if (!regex)
        return NULL;

    char *r = regex;
    *r++ = '^';

    while (*glob) {
        switch (*glob) {
            case '*':
                *r++ = '.';
                *r++ = '*';
                break;
            case '?':
                *r++ = '.';
                break;
            case '.':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '+':
            case '\\':
            case '^':
            case '$':
            case '|':
                *r++ = '\\';
                *r++ = *glob;
                break;
            default:
                *r++ = *glob;
        }
        glob++;
    }

    *r++ = '$';
    *r = '\0';

    return regex;
}

/**
 * Add an exclusion pattern to the configuration
 */
int zp_compile_config_add_exclusion(ZpCompileConfig config, const char *pattern)
{
    if (!config || !pattern)
        return 1;

    /* Expand the array */
    char **new_patterns = (char **)realloc(
        config->exclusion_patterns,
        (config->exclusion_count + 1) * sizeof(char *)
    );

    if (!new_patterns)
        return 1;

    config->exclusion_patterns = new_patterns;

    /* Expand the regex array */
    regex_t *new_regex = (regex_t *)realloc(
        config->exclusion_regex,
        (config->exclusion_count + 1) * sizeof(regex_t)
    );

    if (!new_regex)
        return 1;

    config->exclusion_regex = new_regex;

    /* Add the pattern */
    config->exclusion_patterns[config->exclusion_count] = strdup(pattern);

    /* Compile the regex */
    char *regex_pattern = glob_to_regex(pattern);
    if (!regex_pattern)
        return 1;

    int result = regcomp(
        &config->exclusion_regex[config->exclusion_count],
        regex_pattern,
        REG_EXTENDED | REG_NOSUB
    );

    free(regex_pattern);

    if (result != 0)
        return 1;

    config->exclusion_count++;
    return 0;
}

/**
 * Add an inclusion pattern to the configuration
 */
int zp_compile_config_add_inclusion(ZpCompileConfig config, const char *pattern)
{
    if (!config || !pattern)
        return 1;

    /* Expand the array */
    char **new_patterns = (char **)realloc(
        config->inclusion_patterns,
        (config->inclusion_count + 1) * sizeof(char *)
    );

    if (!new_patterns)
        return 1;

    config->inclusion_patterns = new_patterns;

    /* Expand the regex array */
    regex_t *new_regex = (regex_t *)realloc(
        config->inclusion_regex,
        (config->inclusion_count + 1) * sizeof(regex_t)
    );

    if (!new_regex)
        return 1;

    config->inclusion_regex = new_regex;

    /* Add the pattern */
    config->inclusion_patterns[config->inclusion_count] = strdup(pattern);

    /* Compile the regex */
    char *regex_pattern = glob_to_regex(pattern);
    if (!regex_pattern)
        return 1;

    int result = regcomp(
        &config->inclusion_regex[config->inclusion_count],
        regex_pattern,
        REG_EXTENDED | REG_NOSUB
    );

    free(regex_pattern);

    if (result != 0)
        return 1;

    config->inclusion_count++;
    return 0;
}

/**
 * Check if a file should be excluded from compilation based on patterns
 */
int zp_compile_config_should_exclude(ZpCompileConfig config, const char *file)
{
    if (!config || !file)
        return 1;

    /* Check file size if it's not too large */
    struct stat st;
    if (stat(file, &st) == 0 && S_ISREG(st.st_mode)) {
        if (st.st_size > config->max_file_size)
            return 1;
    }

    /* Check against exclusion patterns */
    for (int i = 0; i < config->exclusion_count; i++) {
        if (regexec(&config->exclusion_regex[i], file, 0, NULL, 0) == 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * Check if a file should be included for compilation (override exclusions)
 */
int zp_compile_config_should_include(ZpCompileConfig config, const char *file)
{
    if (!config || !file)
        return 0;

    /* Check against inclusion patterns */
    for (int i = 0; i < config->inclusion_count; i++) {
        if (regexec(&config->inclusion_regex[i], file, 0, NULL, 0) == 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * Add a file to the pending compilation batch
 */
int zp_compile_config_add_pending(ZpCompileConfig config, const char *file)
{
    if (!config || !file || !config->batch_mode)
        return 1;

    /* Check if we need to allocate more space */
    if (config->pending_count >= config->pending_alloc) {
        int new_alloc = config->pending_alloc ? config->pending_alloc * 2 : 16;
        char **new_files = (char **)realloc(
            config->pending_files,
            new_alloc * sizeof(char *)
        );

        if (!new_files)
            return 1;

        config->pending_files = new_files;
        config->pending_alloc = new_alloc;
    }

    /* Add the file */
    config->pending_files[config->pending_count++] = strdup(file);

    /* Process the batch if it's full */
    if (config->pending_count >= config->batch_size) {
        return zp_compile_config_process_batch(config);
    }

    return 0;
}

/**
 * Process pending compilation batch if conditions are met
 */
int zp_compile_config_process_batch(ZpCompileConfig config)
{
    if (!config || !config->pending_count)
        return 0;

    /* Check if enough time has passed since the last batch */
    time_t now = time(NULL);
    if (now - config->last_batch_time < config->batch_interval)
        return 0;

    /* Process the batch */
    /* In a real implementation, we would spawn a background process to compile all files */
    /* For now, we'll just clear the pending list */

    /* Log the batch if in debug mode */
    if (config->debug_mode) {
        fprintf(stderr, "Processing batch of %d files for compilation\n", config->pending_count);
        for (int i = 0; i < config->pending_count; i++) {
            fprintf(stderr, "  %s\n", config->pending_files[i]);
        }
    }

    /* Clean up */
    zp_compile_config_clear_pending(config);
    config->last_batch_time = now;

    return 0;
}

/**
 * Clear all pending files from the batch
 */
void zp_compile_config_clear_pending(ZpCompileConfig config)
{
    if (!config || !config->pending_files)
        return;

    for (int i = 0; i < config->pending_count; i++) {
        free(config->pending_files[i]);
    }

    config->pending_count = 0;
}
