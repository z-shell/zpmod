# Memory Management and Performance Optimizations

This document details the critical memory management solutions and performance optimizations implemented in zpmod's Phase 1 features.

## Overview

During the implementation of fpath-index persistence and path-warmup enhancements, we encountered and solved two critical issues:

1. **Double-free memory corruption** in path-warmup prune functionality
2. **Unreliable skip detection** in fpath-index due to inconsistent mtime handling

These solutions follow zpmod's established patterns for safe zsh module development and provide important lessons for future development.

## Problem 1: Path-Warmup Memory Corruption

### The Issue

The original path-warmup prune implementation caused double-free corruption when attempting to modify the `$PATH` array. The crash occurred
in this sequence:

```c
// DANGEROUS: This pattern caused memory corruption
// (Attempted in-place modification then reassignment)
// ... modify path array in-place ...
setaparam("path", modified_path);          // Triggers double-free
```

### Root Cause Analysis (Prune)

Through systematic GDB debugging, we identified the issue:

1. `getaparam("path")` returns a reference to zsh's internal parameter storage
2. Attempting to modify this array or create new arrays that reference its strings creates ownership conflicts
3. `setaparam("path", new_array)` tries to free the old array, but some strings are still referenced
4. This leads to double-free corruption in `freearray()` → `arrvarsetfn()` → `assignaparam()`

### Immediate Fault (Prune Bug)

### Solution (Prune): Safe Array Reconstruction

We implemented a memory-safe approach that avoids ownership conflicts:

```c
// SAFE: Independent array construction
char **path = getaparam("path");                    // Read-only access
char **new_path = zalloc((count + 1) * sizeof(char *)); // New allocation

for (int i = 0; path[i]; i++) {
    if (is_valid_directory(path[i])) {
        new_path[new_idx++] = ztrdup(path[i]);      // Independent string copy
    }
}
setaparam("path", new_path);                        // Safe ownership transfer
```

### Key Principles (Prune Summary)

1. **Never modify arrays returned by `getaparam()`** - they're owned by zsh's parameter system

## Problem 2: Fpath-Index Skip Detection

### Symptoms (Skip Detection)

The fpath-index skip detection was unreliable, causing unnecessary rebuilds when directories hadn't actually changed. The test failure was:

```text
file unexpectedly rewritten (should skip)
```

### Root Cause Investigation (Skip Detection)

The issue was in inconsistent mtime handling for missing directories:

1. **During index generation**: `stat()` fails for missing directory → record `-1`
2. **During skip verification**: `stat()` succeeds (or behaves differently) → get real mtime
3. **Comparison**: `-1` ≠ real mtime → force rebuild unnecessarily

This happened because:

- Directory existence could change between runs
- `stat()` behavior could vary based on filesystem state or permissions
- The comparison logic didn't handle missing directories consistently

### Solution (Skip Detection): Unified Mtime Handling

We implemented consistent mtime handling for both generation and verification:

```c
// CONSISTENT: Same logic for both generation and verification
long get_directory_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return (long)st.st_mtime;
    }
    return -1;  // Consistent sentinel for missing/inaccessible directories
}

// During generation:
long mtime = get_directory_mtime(dir);
fprintf(index_file, "# dir %d %s %ld\n", i, dir, mtime);

// During verification:
long current_mtime = get_directory_mtime(dir);
if (current_mtime != recorded_mtime) {
    header_ok = 0;  // Trigger rebuild
}
```

### Skip Detection Algorithm

The enhanced skip detection now follows this logic:

1. **Format validation**: Verify index file format version
2. **Complete coverage**: Ensure header covers all current fpath entries
3. **Sequence validation**: Verify directory ordering matches current fpath
4. **Change detection**: Compare mtimes using consistent missing-directory handling
5. **Skip decision**: Skip only if ALL validations pass

## Memory Management Best Practices

Based on these solutions, we've established these patterns for safe zpmod development:

### String Handling

```c
// DO: Use zsh allocators for strings
char *safe_copy = ztrdup(original_string);
// Later: zsfree(safe_copy) if manual cleanup needed

// DON'T: Mix malloc/free with zsh strings
char *unsafe = strdup(zsh_string);  // WRONG: incompatible allocators
```

### Array Management

```c
// DO: Create independent arrays
char **new_array = zalloc((count + 1) * sizeof(char *));
for (int i = 0; i < count; i++) {
    new_array[i] = ztrdup(source[i]);  // Independent ownership
}
setaparam("name", new_array);  // Transfer ownership to zsh

// DON'T: Reuse getaparam() results
char **array = getaparam("name");
array[0] = "modified";  // WRONG: modifying zsh-owned memory
setaparam("name", array);  // WRONG: double ownership
```

### Parameter Access Patterns

```c
// DO: Treat getaparam() results as read-only
char **readonly_array = getaparam("name");
// Use for reading only, never modify

// DO: Use separate allocation for modifications
char **writable_copy = create_array_copy(readonly_array);
// Modify writable_copy, then setaparam()
```

## Performance Implications

These solutions maintain or improve performance:

### Path-Warmup

- **Prune efficiency**: Two-pass algorithm minimizes allocations
- **Memory usage**: Exact allocation sizing reduces waste
- **Cache locality**: Linear scan patterns optimize for filesystem caches

### Fpath-Index

- **Skip detection**: Prevents expensive directory scans when unchanged
- **Preload optimization**: Populates function table without file I/O when skipping
- **Startup impact**: Significantly reduces shell startup time with large fpaths

## Testing Strategies

Our testing approach ensures these solutions remain stable:

### Memory Safety Testing

- **Systematic reduction**: Isolate memory issues by progressively simplifying code
- **GDB analysis**: Use detailed backtraces to identify ownership conflicts
- **Valgrind integration**: Automated detection of memory errors in CI

### Skip Detection Testing

- **Timestamp preservation**: Verify mtimes are consistent between runs
- **Change detection**: Ensure directory modifications trigger rebuilds
- **Edge cases**: Test missing directories, permission changes, etc.

## Future Considerations

These patterns should guide future zpmod development:

1. **Always use zsh allocators** for strings and arrays that interact with parameters
2. **Treat getaparam() as read-only** - never modify returned arrays
3. **Implement consistent state handling** for optional/missing resources
4. **Design for ownership clarity** - each string/array should have clear ownership
5. **Test edge cases thoroughly** - missing directories, permission changes, etc.

These solutions provide a foundation for reliable, high-performance shell module development within zsh's complex parameter and memory
management system.
