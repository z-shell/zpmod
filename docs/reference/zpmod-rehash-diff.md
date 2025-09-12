# `zpmod rehash-diff`

Incremental alternative to `rehash` that will, when fully implemented, only rescan PATH segments that are new or whose contents changed.

## Planned Behavior

1. Maintain a snapshot (hash) per PATH directory of mtime+inode of entries
2. On invocation, diff current PATH vs snapshot (added, removed, reordered)
3. Rehash only affected directories, updating the shell's command hash table
4. Persist snapshot for subsequent runs (XDG cache)

## Current Implementation (initial slice)

Persists a snapshot of PATH directories (inode+mtime) and reports added / removed / changed segments.

## CLI

```sh
zpmod rehash-diff
```
