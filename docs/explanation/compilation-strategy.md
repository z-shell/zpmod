# Compilation & Caching Strategy

## Goal

Minimize startup and sourcing time by using zsh's wordcode compiled form (`.zwc`) while remaining transparent to users.

## Decision Flow

1. Intercept `source` / `.` call.
2. Determine candidate script path and potential `.zwc` sibling.
3. If existing `.zwc` is newer or equal timestamp → load compiled via `custom_check_dump_file`.
4. Else if script writable OR debug mode (`ZI_MOD_DEBUG=1`) → invoke `zcompile` equivalent path to produce new `.zwc`.
5. Retry load.
6. Fallback: interpret script.

## Freshness Check

`stat()` timestamps compare source vs compiled file modification times.

## Debug Mode Influence

When `ZI_MOD_DEBUG=1` module logs reasons for skipping compilation (permissions, missing access) and may attempt compilation even if not strictly necessary for diagnostics.

## Error Handling

- Missing file: returns SOURCE_NOT_FOUND → builtin emits warning / error depending on POSIX mode.
- Corrupt `.zwc`: module warns (if debug) and falls back to interpretive path.
