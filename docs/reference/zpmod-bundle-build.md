# `zpmod bundle-build`

Builds a startup bundle by concatenating plugin/script files so they can be `zcompile`d once, reducing startup overhead.

## Planned Behavior

| Step     | Description                                                                 |
| -------- | --------------------------------------------------------------------------- |
| Collect  | Find candidate scripts (`*.zsh`, `*.plugin.zsh`) recursively (configurable) |
| Sort     | Deterministic lexical ordering (locale C)                                   |
| Filter   | Enforce optional size cap (`--max KB`) before writing                       |
| Emit     | Write bundle with BEGIN/END markers + original relative path comments       |
| Optimize | Optionally strip trailing whitespace / comments (flagged)                   |
| Persist  | Rebuild only if any source file newer than bundle (mtime compare)           |

## CLI

```zsh
zpmod bundle-build --from <dir> --out <bundle.zsh> [--max <KB>]
```

## Integration Example

```zsh
if command -v zpmod >/dev/null; then
  bundle_dir="$HOME/.cache/zpmod/bundles"
  mkdir -p "$bundle_dir"
  bundle="$bundle_dir/startup.zsh"
  zpmod bundle-build --from "$HOME/.zsh_plugins" --out "$bundle" --max 256
  [[ -f $bundle.zwc && $bundle.zwc -nt $bundle ]] || zcompile "$bundle"
  source "$bundle"  # or autoload functions inside
fi
```
