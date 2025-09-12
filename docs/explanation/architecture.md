# Architecture

zpmod is a binary zsh module providing two primary enhancements:

1. Opportunistic compilation of sourced scripts to `.zwc` and subsequent fast loading.
2. Comprehensive profiling of every sourced file during shell initialization.

## Layout

The codebase is split into clear layers:

- `src/core/` — core facilities used across commands
  - `utils.c`: unmetafy/dup helpers and argv scanning
  - `emoji.c`: terminal/locale detection and icon helpers
  - `fs.c`: filesystem primitives (path-stat, dir-list, read-file)
  - `source.c`: custom dot/source override and source-study implementation
- `src/builtins/` — thin wrappers that expose functionality as builtins
  - `zpmod_builtin.c`: the `zpmod` command (subcommands: report-append, source-study, dir-list, path-stat, read-file)
  - `fs_builtins.c`: `zppathstat`, `zpdirlist`, `zpreadfile`
  - `readarray.c`: `readarray` builtin
- `src/compat/` — cross-version shims
  - `options.c`: stable-to-runtime option mapping for varying zsh versions
- `src/include/` — public headers wiring modules together
  - `zpmod_*.h` headers export small, focused interfaces between units
- `src/module/module.c` — builtin table and module hooks (setup*/finish*, features\_, ...)
- `src/module/zpmod.mdh`, `src/module/zpmod.pro` — out-of-tree build stubs
- `src/completion/_zpmod` — zsh completion script installed with the module

This replaces the older monolithic `src/zpmod.c` with modular units that are easier to navigate and test.

> See also: a concise maintainer-oriented overview of the `src/` layout and rules lives in [src/README.md](../../src/README.md).

## Hooks

At `setup_()` the module:

- Locates builtin entries for `.` and `source`.
- Substitutes their handlers with `bin_custom_dot`.
- Keeps original function pointers for restoration in `finish_()`.

## Event Tracking (Source Study)

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
