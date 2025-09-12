# Implementing Skip Detection for Performance Optimization

This guide explains how to implement reliable skip detection mechanisms for expensive operations in zpmod, based on the fpath-index skip
detection solution.

## Overview

Skip detection allows commands to avoid expensive operations (like directory scanning) when the underlying data hasn't changed. This is
crucial for performance in shell environments where commands may be run frequently.

## When to Use Skip Detection

Skip detection is beneficial when:

- **Operation is expensive**: Directory scanning, file parsing, network requests
- **Input changes infrequently**: System paths, configuration files, cached data
- **Accuracy is critical**: False skips cause stale data, false rebuilds waste performance
- **State is verifiable**: Changes can be detected through timestamps, checksums, etc.

## Basic Skip Detection Pattern

### 1. State Recording

When generating output, record the state of inputs:

```c
// Record input state alongside output data
FILE *output = fopen(output_file, "w");
fprintf(output, "# format-version v1\n");

// Record each input with its state identifier
for (int i = 0; inputs[i]; i++) {
    struct stat st;
    long mtime = -1;  // Sentinel for missing/inaccessible
    if (stat(inputs[i], &st) == 0) {
        mtime = (long)st.st_mtime;
    }
    fprintf(output, "# input %d %s %ld\n", i, inputs[i], mtime);
}

// Generate actual output data
generate_output_data(output, inputs);
fclose(output);
```

### 2. State Verification

When checking if skip is possible, verify all recorded state:

```c
bool can_skip_operation(const char *output_file, char **current_inputs) {
    FILE *input = fopen(output_file, "r");
    if (!input) return false;  // No previous output exists

    char line[2048];

    // Verify format version
    if (!fgets(line, sizeof(line), input) ||
        strncmp(line, "# format-version v1", 19) != 0) {
        fclose(input);
        return false;
    }

    int idx = 0;
    bool state_valid = true;

    // Verify each recorded input state
    while (fgets(line, sizeof(line), input) && state_valid) {
        if (strncmp(line, "# input ", 8) != 0) {
            break;  // End of state section
        }

        char tag[16];
        int recorded_idx;
        char recorded_path[2048];
        long recorded_mtime;

        if (sscanf(line, "# %15s %d %2047s %ld",
                   tag, &recorded_idx, recorded_path, &recorded_mtime) != 4) {
            state_valid = false;
            break;
        }

        // Verify input order and existence
        if (strcmp(tag, "input") != 0 || recorded_idx != idx ||
            !current_inputs[idx] || strcmp(current_inputs[idx], recorded_path) != 0) {
            state_valid = false;
            break;
        }

        // Verify input state hasn't changed
        long current_mtime = get_mtime_safe(current_inputs[idx]);
        if (current_mtime != recorded_mtime) {
            state_valid = false;
            break;
        }

        idx++;
    }

    // Ensure we covered all current inputs
    if (state_valid && current_inputs[idx] != NULL) {
        state_valid = false;  // New inputs added
    }

    fclose(input);
    return state_valid;
}
```

### 3. Safe State Extraction

Implement consistent state extraction to avoid the mtime inconsistency problem:

```c
/**
 * Extract mtime safely, handling missing directories consistently.
 * Returns -1 for missing/inaccessible paths, actual mtime otherwise.
 */
long get_mtime_safe(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return (long)st.st_mtime;
    }
    return -1;  // Consistent sentinel value
}
```

## Advanced Patterns

### Hierarchical Skip Detection

For complex operations with multiple levels of dependencies:

```c
typedef struct {
    char *path;
    long mtime;
    int file_count;  // Additional state for directories
} input_state_t;

bool verify_hierarchical_state(input_state_t *recorded, char **current_paths) {
    for (int i = 0; current_paths[i]; i++) {
        // Basic mtime check
        long current_mtime = get_mtime_safe(current_paths[i]);
        if (current_mtime != recorded[i].mtime) {
            return false;
        }

        // Additional checks for directories
        if (is_directory(current_paths[i])) {
            int current_file_count = count_files_in_directory(current_paths[i]);
            if (current_file_count != recorded[i].file_count) {
                return false;  // Directory contents changed
            }
        }
    }
    return true;
}
```

### Content-Based Skip Detection

For files where mtime isn't sufficient (e.g., files that may be touched without content changes):

```c
#include <openssl/md5.h>  // or use simpler hash

typedef struct {
    char *path;
    unsigned char hash[MD5_DIGEST_LENGTH];
} content_state_t;

bool verify_content_state(content_state_t *recorded, char **current_paths) {
    for (int i = 0; current_paths[i]; i++) {
        unsigned char current_hash[MD5_DIGEST_LENGTH];
        if (!compute_file_hash(current_paths[i], current_hash)) {
            return false;  // Can't verify
        }

        if (memcmp(current_hash, recorded[i].hash, MD5_DIGEST_LENGTH) != 0) {
            return false;  // Content changed
        }
    }
    return true;
}
```

## Error Handling and Edge Cases

### Missing Files

```c
long get_mtime_safe(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return (long)st.st_mtime;
    }

    // Don't differentiate between different failure reasons
    // This ensures consistent behavior across filesystem states
    return -1;
}
```

### Permission Changes

```c
typedef struct {
    long mtime;
    mode_t mode;  // Include permissions in state
} extended_state_t;

bool verify_extended_state(const char *path, extended_state_t *recorded) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return recorded->mtime == -1;  // Both should be inaccessible
    }

    return (long)st.st_mtime == recorded->mtime &&
           st.st_mode == recorded->mode;
}
```

### Format Versioning

Always include format version for future compatibility:

```c
#define CURRENT_FORMAT_VERSION "v2"

bool check_format_compatibility(FILE *input) {
    char line[256];
    if (!fgets(line, sizeof(line), input)) {
        return false;
    }

    // Support multiple versions
    if (strncmp(line, "# format-version v1", 19) == 0 ||
        strncmp(line, "# format-version v2", 19) == 0) {
        return true;
    }

    return false;  // Unsupported format
}
```

## Testing Skip Detection

### Basic Functionality Tests

```bash
#!/usr/bin/env zsh

# Test 1: Initial generation
output=$(my_command --output /tmp/test.out input1 input2)
[[ -f /tmp/test.out ]] || fail "Output not generated"

# Test 2: Skip detection works
timestamp_before=$(stat -c %Y /tmp/test.out)
sleep 1
my_command --output /tmp/test.out input1 input2
timestamp_after=$(stat -c %Y /tmp/test.out)
[[ $timestamp_before -eq $timestamp_after ]] || fail "File rewritten unnecessarily"

# Test 3: Change detection works
touch input1  # Modify input
my_command --output /tmp/test.out input1 input2
timestamp_changed=$(stat -c %Y /tmp/test.out)
[[ $timestamp_changed -ne $timestamp_before ]] || fail "Changes not detected"
```

### Edge Case Tests

```bash
# Test missing inputs
mkdir -p /tmp/test_inputs
my_command --output /tmp/test.out /tmp/test_inputs/exists /tmp/test_inputs/missing

# Remove directory and test again
rmdir /tmp/test_inputs
my_command --output /tmp/test.out /tmp/test_inputs/exists /tmp/test_inputs/missing

# Recreate and verify behavior
mkdir -p /tmp/test_inputs
my_command --output /tmp/test.out /tmp/test_inputs/exists /tmp/test_inputs/missing
```

## Performance Considerations

### I/O Optimization

```c
// Minimize stat() calls by caching results
static struct {
    char *path;
    struct stat cached_stat;
    time_t cache_time;
} stat_cache[64];

struct stat *get_stat_cached(const char *path) {
    time_t now = time(NULL);

    // Check cache first
    for (int i = 0; i < cache_count; i++) {
        if (stat_cache[i].path && strcmp(stat_cache[i].path, path) == 0) {
            if (now - stat_cache[i].cache_time < 1) {  // 1 second cache
                return &stat_cache[i].cached_stat;
            }
        }
    }

    // Cache miss - populate cache
    // ... implementation
}
```

### Memory Efficiency

```c
// Use memory pools for temporary allocations during verification
typedef struct {
    char *buffer;
    size_t size;
    size_t used;
} memory_pool_t;

memory_pool_t *create_temp_pool(size_t initial_size) {
    memory_pool_t *pool = zalloc(sizeof(*pool));
    pool->buffer = zalloc(initial_size);
    pool->size = initial_size;
    pool->used = 0;
    return pool;
}

char *pool_strdup(memory_pool_t *pool, const char *str) {
    size_t len = strlen(str) + 1;
    if (pool->used + len > pool->size) {
        // Expand pool or return NULL
        return NULL;
    }

    char *result = pool->buffer + pool->used;
    strcpy(result, str);
    pool->used += len;
    return result;
}
```

## Integration with zpmod

When implementing skip detection in zpmod commands:

1. **Follow zpmod patterns**: Use zsh allocators, proper error reporting
2. **Respect command-line flags**: Implement `--rebuild` to force regeneration
3. **Handle preload modes**: Support operations that populate data structures without file I/O
4. **Test thoroughly**: Include edge cases in the test suite

This pattern provides reliable performance optimization while maintaining data consistency and following zpmod's established conventions.
