# Architecture

zpmod is a binary zsh module providing two primary enhancements:

1. Opportunistic compilation of sourced scripts to `.zwc` and subsequent fast loading.
2. Comprehensive profiling of every sourced file during shell initialization.

## Hooks

At `setup_()` the module:

- Locates builtin entries for `.` and `source`.
- Substitutes their handlers with `bin_custom_dot`.
- Keeps original function pointers for restoration in `finish_()`.

## Event Tracking

Each time a file is sourced via intercepted builtins:

- Start timestamp recorded
- Attempt to load compiled Eprog (existing or after on-demand zcompile)
- Execute with state preservation
- End timestamp recorded
- Hashtable entry keyed by incrementing ID captures: directory, file, full path, duration, status.

## Memory & Safety

- String allocations use zsh allocators (zalloc/zsfree) to integrate with shell GC expectations.
- Profiling report builder uses incremental buffer growth (zrealloc) to minimize fragmentation.

## Option Compatibility

The module builds an internal stable-to-runtime option index mapping to insulate from zsh’s shifting option enumeration across versions.
