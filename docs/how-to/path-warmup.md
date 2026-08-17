# PATH warmup (fast rehash alternative)

The zpmod subcommand `path-warmup` performs a quick scan of your `$PATH` directories and touches executable entries to let the OS kernel
warm filesystem metadata caches. This helps reduce latency on the first command lookup without doing a full `rehash`.

## Usage

- Quiet (recommended):

  ```zsh
  zpmod path-warmup -q
  ```

- Optional prune flags:

  ```zsh
  # remove non-existent PATH entries from $path
  zpmod path-warmup -q --prune-missing

  # show what would be pruned without modifying $path
  zpmod path-warmup -q --prune-missing --dry-run
  ```

The command is safe and idempotent; running it multiple times does not change shell state.

## Zi integration snippet

To run this early with Zi, add to your `.zshrc` (after PATH is finalized):

```zsh
zi ice wait'0'
zi snippet 'https://gist.github.com/your-org-or-user/xyz' # optional, or inline function
() {
  # Enable optional FS cache layer during session (prototype)
  export ZPMOD_FS_CACHE=1
  # Warm PATH metadata caches
  zpmod path-warmup -q || true
}
```

Alternatively, use `atinit` hook within a Zi plugin:

```zsh
zi for \
  atinit'export ZPMOD_FS_CACHE=1; zpmod path-warmup -q || true' \
  z-shell/zpmod
```

Notes:

- The FS cache is a small, opt-in prototype (dir listings); disable by unsetting `ZPMOD_FS_CACHE`.
- This does not alter zsh's command hash table directly; it warms the kernel caches that speed up subsequent lookups.
- With `--prune-missing`, missing directories are removed from the shell's `$path` unless `--dry-run` is given.

## Example: Zi

With `zi`, you can warm up the PATH and enable the FS cache with a one-liner in your `.zshrc`:

```zsh
# .zshrc
zi ice atinit'zpmod_config[FS_CACHE]=1; zpmod path-warmup -q || true' \
    load'z-shell/zpmod'
```

This does a few things:

- The FS cache is a small, opt-in prototype (dir listings); disable by unsetting `ZPMOD[FS_CACHE]`.
- `path-warmup` is run quietly in the background.
- Failures are ignored (`|| true`) to avoid blocking shell startup.
