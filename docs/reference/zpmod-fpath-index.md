# Fpath-Index Enhanced Reference

## Synopsis

```bash
zpmod fpath-index [--out FILE] [--rebuild] [--preload]
```

## Description

The fpath-index command generates function-to-path mappings from FPATH directories with intelligent skip detection for optimal performance.
This enhanced implementation provides reliable caching that avoids unnecessary regeneration when FPATH directories haven't changed.

## Options

- `--out FILE`: Write index to specified file instead of stdout
- `--rebuild`: Force regeneration even if existing index is fresh
- `--preload`: Populate shell function table without writing index file

## Features

### Function Discovery

Scans all directories in `$FPATH` to discover available shell functions and their locations:

**Example output:**

```bash
$ zpmod fpath-index
# zpmod fpath-index v1
# dir 0 /usr/share/zsh/functions/Completion 1234567890
# dir 1 /usr/share/zsh/functions/Misc 1234567891
_complete /usr/share/zsh/functions/Completion/_complete
zcalc /usr/share/zsh/functions/Misc/zcalc
zed /usr/share/zsh/functions/Misc/zed
```

### Intelligent Skip Detection (Enhanced)

Automatically detects when existing indexes are still fresh, avoiding expensive directory scanning:

```bash
$ zpmod fpath-index --out ~/.cache/fpath.idx
# First run: generates index

$ zpmod fpath-index --out ~/.cache/fpath.idx
# Second run: skips generation (file unchanged)

$ touch /usr/share/zsh/functions/Misc/new_function
$ zpmod fpath-index --out ~/.cache/fpath.idx
# Third run: regenerates (directory changed)
```

### Preload Mode

Populates the shell's function table for faster autoloading without file I/O:

```bash
$ zpmod fpath-index --preload
# Functions now available for autoload without scanning directories
```

## Index Format

The enhanced fpath-index uses a structured format (v1) for reliable skip detection:

```text
# zpmod fpath-index v1
# dir 0 /first/fpath/dir 1234567890
# dir 1 /second/fpath/dir 1234567891
# dir 2 /third/fpath/dir -1
function_name /first/fpath/dir/function_name
other_func /second/fpath/dir/other_func
```

### Header Section

- **Format line**: `# zpmod fpath-index v1` - identifies format version
- **Directory lines**: `# dir <index> <path> <mtime>` - records directory state
  - `<index>`: Sequential directory number matching FPATH order
  - `<path>`: Full directory path
  - `<mtime>`: Directory modification time, or `-1` if inaccessible

### Function Section

- **Function lines**: `<name> <full_path>` - maps function names to file paths
- Functions starting with `.` or `_` may be filtered based on conventions

## Implementation Details

### Skip Detection Algorithm

The enhanced skip detection prevents unnecessary rebuilds through comprehensive validation:

1. **Format verification**: Ensures index file uses supported format version
2. **Complete coverage**: Validates that header covers all current FPATH entries
3. **Sequence validation**: Confirms directory ordering matches current FPATH
4. **Change detection**: Compares directory mtimes using consistent missing-directory handling
5. **Skip decision**: Skips regeneration only if ALL validations pass

### Consistent Mtime Handling

The critical enhancement ensures missing directories are handled identically during generation and verification:

```c
// Both generation and verification use identical logic:
long get_directory_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return (long)st.st_mtime;
    }
    return -1;  // Consistent sentinel for missing/inaccessible directories
}
```

This prevents false rebuild triggers when directory accessibility varies between runs.

### Preload Integration

When skip detection succeeds but preload is requested, the command:

1. Scans directories to discover functions (using cached index state)
2. Populates `shfunctab` with function stubs
3. Provides performance benefit without file I/O overhead

## Examples

### Basic Index Generation

Generate function index to stdout:

```bash
zpmod fpath-index
```

### Persistent Indexing

Create and maintain a persistent index file:

```bash
# Initial generation
zpmod fpath-index --out ~/.cache/fpath.idx

# Later runs automatically skip if unchanged
zpmod fpath-index --out ~/.cache/fpath.idx
```

### Force Regeneration

Bypass skip detection when needed:

```bash
zpmod fpath-index --out ~/.cache/fpath.idx --rebuild
```

### Function Preloading

Load functions into shell without generating file:

```bash
zpmod fpath-index --preload
```

### Combined Operations

Generate index and preload simultaneously:

```bash
zpmod fpath-index --out ~/.cache/fpath.idx --preload
```

## Integration

### Shell Startup Optimization

Add to `.zshrc` for faster function loading:

```bash
# Fast function preloading with persistent cache
if (( ${+commands[zpmod]} )); then
    zpmod fpath-index --out ~/.cache/fpath.idx --preload
fi
```

### Build System Integration

Use in build scripts for function discovery:

```bash
#!/usr/bin/env zsh
# generate-function-manifest.zsh

zpmod fpath-index --out build/function-manifest.txt

# Parse manifest for build system
grep -v '^#' build/function-manifest.txt | while read func path; do
    echo "Found function: $func at $path"
done
```

### Cache Management

Implement cache invalidation logic:

```bash
#!/usr/bin/env zsh
# smart-fpath-cache.zsh

cache_file="$HOME/.cache/fpath.idx"
max_age=$((24 * 60 * 60))  # 24 hours

if [[ ! -f $cache_file ]] || (( $(date +%s) - $(stat -c %Y $cache_file) > max_age )); then
    zpmod fpath-index --out $cache_file --rebuild --preload
else
    zpmod fpath-index --out $cache_file --preload
fi
```

## Performance Characteristics

### Skip Detection Benefits

- **Startup optimization**: Avoids directory scanning when FPATH unchanged
- **Cache locality**: Minimizes filesystem access for repeated operations
- **Scalability**: Performance improvement increases with FPATH size

### Generation Costs

- **Directory scanning**: Linear in number of directories and functions
- **File I/O**: Proportional to index file size
- **Memory usage**: Temporary allocations for function discovery

### Optimization Strategies

- **Persistent caching**: Use `--out` to avoid regeneration
- **Preload mode**: Populate function table without file I/O when skipping
- **Strategic placement**: Put frequently-used function directories early in FPATH

## Exit Status

- **0**: Success (whether generated, skipped, or preloaded)
- **1**: Error (invalid arguments, I/O failure, etc.)

## Environment Variables

- `FPATH`: The directory list to index
- `ZPMOD_FS_CACHE`: Enable filesystem caching optimizations (if set and non-zero)

## Compatibility

### Shell Compatibility

This command is specific to zsh and requires:

- zsh parameter system access (`FPATH`)
- Shell function table integration (`shfunctab`)
- zpmod module loaded

### Function Discovery Rules

- Scans for executable files in FPATH directories
- Excludes files starting with `.` (hidden files)
- May exclude files starting with `_` based on conventions
- Follows symbolic links for function discovery

## File Format Evolution

### Version 1 (Current)

```bash
# zpmod fpath-index v1
# dir <index> <path> <mtime>
<function> <path>
```

### Future Compatibility

The format version header enables future enhancements:

- Additional metadata in headers
- Function attribute recording
- Compression or binary formats
- Cross-session caching improvements

## Troubleshooting

### Skip Detection Issues

**Index always regenerates:** Check if FPATH directories are frequently modified or if filesystem mtimes are unreliable.

**False skips with stale data:** Use `--rebuild` to force regeneration and investigate skip detection logic.

### Performance Problems

**Slow generation:** Monitor directories with many files or slow filesystem access.

**Memory usage:** Large FPATH configurations may require substantial memory for function discovery.

### Cache Corruption

**Invalid index format:** Delete cache files and regenerate:

```bash
rm ~/.cache/fpath.idx
zpmod fpath-index --out ~/.cache/fpath.idx
```

## See Also

- [`zpmod(1)`](zpmod.1.md) - Main zpmod command reference
- [`zpmod-path-warmup(1)`](zpmod-path-warmup.1.md) - PATH optimization functionality
- [Memory Management Solutions](../explanation/memory-management-solutions.md) - Implementation details
- [Implementing Skip Detection](../how-to/implementing-skip-detection.md) - Skip detection patterns
