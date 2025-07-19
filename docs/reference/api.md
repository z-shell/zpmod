# zpmod API Reference

## Introduction

This document provides a detailed technical reference for the zpmod Zsh module, including its commands, functions, and environment variables.

## Commands

### `zpmod source-study`

Displays performance data for sourced files.

#### Options

- `-l`: Show full file paths instead of just filenames
- `-s`: Sort by load time (slowest first)
- `-n <number>`: Show only the top N entries

#### Example

```zsh
# Show basic report
zpmod source-study

# Show detailed report with full paths
zpmod source-study -l

# Show top 10 slowest files
zpmod source-study -s -n 10
```

## Environment Variables

### `ZPMOD_DEBUG`

When set to `1`, enables debug logging (if compiled with debug support).

Example:

```zsh
export ZPMOD_DEBUG=1
```

### `ZPMOD_SKIP_PATTERNS`

Array of patterns to skip during compilation (requires custom build).

Example:

```zsh
export ZPMOD_SKIP_PATTERNS=("*.config" "*/temp/*")
```

## Internal Functions

These functions are part of the module's implementation and not meant to be called directly.

### `zi_check_file`

Checks if a file should be compiled and handles compilation if necessary.

### `zi_track_source`

Tracks the sourcing of files for performance analysis.

## Data Structures

### Source Tracking Data

The module maintains an internal database of sourced files with the following information:

- File path
- Load time (in microseconds)
- Size
- Compilation status
- Last access time

This data is used by the `source-study` command to generate performance reports.
