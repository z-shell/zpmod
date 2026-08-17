# Path-Warmup Enhanced Reference

## Synopsis

```bash
zpmod path-warmup [--quiet|-q] [--prune-missing] [--dry-run]
```

## Description

The path-warmup command optimizes shell performance by warming filesystem caches and maintaining clean PATH configurations. This enhanced
implementation provides safe path pruning functionality alongside the original executable discovery features.

## Options

- `--quiet`, `-q`: Suppress progress messages
- `--prune-missing`: Remove non-existent directories from PATH
- `--dry-run`: Report what would be pruned without making changes (requires `--prune-missing`)

## Features

### Executable Discovery

Scans all directories in `$PATH` and touches executable files to warm filesystem caches. This improves performance of command lookups and
tab completion.

**Example:**

```bash
$ zpmod path-warmup
zpmod: path-warmup scanning 12 directories...
zpmod: path-warmup touched 2847 executables.
```

### Path Pruning (Enhanced)

Safely removes non-existent or inaccessible directories from `$PATH` to reduce lookup overhead and eliminate stale entries that accumulate
over time.

**Dry-run mode:**

```bash
$ zpmod path-warmup --prune-missing --dry-run
zpmod: path-warmup would prune missing directory: /opt/old-software/bin
zpmod: path-warmup would prune missing directory: /home/user/.local/bin
zpmod: path-warmup touched 2847 executables.
```

**Actual pruning:**

```bash
$ zpmod path-warmup --prune-missing
zpmod: path-warmup pruning missing directory: /opt/old-software/bin
zpmod: path-warmup pruning missing directory: /home/user/.local/bin
zpmod: path-warmup touched 2847 executables.
```

## Implementation Details

### Memory Safety

The enhanced path-warmup implementation uses a memory-safe approach for PATH modification:

1. **Read-only access**: Treats `getaparam("path")` results as immutable
2. **Independent allocation**: Creates new arrays with proper string ownership
3. **Safe ownership transfer**: Uses `setaparam()` without dependency conflicts

This design prevents the memory corruption issues that occurred in earlier implementations.

#### Path Pruning & Executable Discovery Characteristics

- **Two-pass pruning**: Minimizes allocations by counting valid directories first
- **Linear scanning**: Optimizes for filesystem cache locality during executable discovery
- **Exact allocation**: Reduces memory waste through precise array sizing

#### Algorithm Walkthrough (Execution + Pruning)

#### Executable Discovery Loop

```text
for each directory in PATH:
    if directory exists and is accessible:
        for each file in directory:
            if file is regular and executable:
                touch file (warm cache)
                increment counter
    close directory
report total count
```

#### Safe Path Pruning

```text
Phase 1 - Analysis:
    valid_count = 0
    for each directory in PATH:
        if directory exists and is accessible:
            increment valid_count
        else if not quiet:
            report pruning action

Phase 2 - Reconstruction (if not dry-run and pruning needed):
    allocate new_path[valid_count + 1]
    new_index = 0
    for each directory in PATH:
        if directory exists and is accessible:
            new_path[new_index++] = ztrdup(directory)  // independent copy
    new_path[new_index] = NULL
    setaparam("path", new_path)  // transfer ownership
```

## Examples

### Basic Usage

Warm filesystem caches for faster command lookups:

```bash
zpmod path-warmup
```

### Quiet Operation

Suppress progress messages for use in scripts:

```bash
zpmod path-warmup --quiet
```

### Path Maintenance

Remove stale directories from PATH:

```bash
# Check what would be removed
zpmod path-warmup --prune-missing --dry-run

# Actually remove stale entries
zpmod path-warmup --prune-missing
```

### Combined Operation

Warm caches and clean PATH in one operation:

```bash
zpmod path-warmup --prune-missing --quiet
```

## Integration

### Shell Startup

Add to `.zshrc` for automatic PATH maintenance:

```bash
# Warm caches and clean PATH on startup
if (( ${+commands[zpmod]} )); then
    zpmod path-warmup --prune-missing --quiet
fi
```

### Periodic Maintenance

Create a maintenance script for regular PATH cleanup:

```bash
#!/usr/bin/env zsh
# path-maintenance.zsh

# Only prune if PATH is "dirty" (has missing entries)
if zpmod path-warmup --prune-missing --dry-run 2>&1 | grep -q "would prune"; then
    echo "Cleaning PATH..."
    zpmod path-warmup --prune-missing
else
    echo "PATH is clean"
    zpmod path-warmup --quiet
fi
```

## Exit Status

- **0**: Success
- **1**: Error (invalid arguments, permissions, etc.)
- **Positive number**: Number of executables discovered (when successful)

## Environment Variables

The path-warmup command respects these environment variables:

- `PATH`: The directory list to process
- `ZPMOD_FS_CACHE`: Enable filesystem caching optimizations (if set and non-zero)

## Compatibility

### Shell Compatibility

This command is specific to zsh and requires the zpmod module. It modifies the `PATH` parameter using zsh's parameter system.

### Filesystem Compatibility

- Works with all POSIX-compliant filesystems
- Handles network filesystems gracefully (may be slower)
- Respects filesystem permissions and access controls
- Safe with symlinks (follows them for directory existence checks)

## Performance Impact

### Benefits

- **Faster command lookups**: Warmed filesystem caches improve initial command discovery
- **Reduced PATH overhead**: Fewer directories to search means faster lookups
- **Improved tab completion**: Warmed caches speed up completion generation

### Costs

- **Initial overhead**: Directory scanning takes time proportional to PATH size
- **Memory usage**: Temporary allocations during PATH reconstruction
- **Disk I/O**: Touching executables generates filesystem activity

### Recommendations

- Run during shell startup when the cost is acceptable
- Use `--quiet` in automated scripts to avoid output noise
- Consider running only periodically rather than on every shell startup
- Monitor impact on slow filesystems (network mounts, etc.)

## Troubleshooting

### Common Issues

**Permission denied errors:**

```bash
zpmod: path-warmup pruning missing directory: /restricted/path
```

This is normal - directories that can't be accessed are considered "missing" for PATH purposes.

**No pruning needed:** If no output appears with `--prune-missing`, your PATH is already clean.

**Performance concerns:** If path-warmup is slow, check for:

- Very large directories in PATH (consider removing or optimizing)
- Network-mounted filesystems in PATH (may need special handling)
- Filesystem issues (check `dmesg` for disk errors)

### Debugging

Enable verbose output to understand what's happening:

```bash
# See what directories are being processed
zpmod path-warmup --prune-missing --dry-run 2>&1

# Check PATH contents
echo $PATH | tr ':' '\n' | nl
```

## See Also

- [`zpmod(1)`](zpmod.1.md) - Main zpmod command reference
- [`zpmod-fpath-index(1)`](zpmod-fpath-index.1.md) - FPATH indexing functionality
- [Memory Management Solutions](../explanation/memory-management-solutions.md) - Implementation details
- [Implementing Skip Detection](../how-to/implementing-skip-detection.md) - Related optimization patterns
