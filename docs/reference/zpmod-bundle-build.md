# `zpmod bundle-build`

Builds a startup bundle by concatenating plugin/script files so they can be `zcompile`d once, reducing startup overhead.

## Behavior

| Step    | Description                                                                 |
| ------- | --------------------------------------------------------------------------- |
| Collect | Find candidate scripts (`*.zsh`, `*.plugin.zsh`) recursively (configurable) |
| Sort    | Deterministic lexical ordering (locale C)                                   |
| Filter  | Enforce optional size cap (`--max KB`) before writing                       |
| Emit    | Write bundle with BEGIN/END markers + original relative path comments       |
| Persist | Rebuild only if any source file newer than bundle (mtime compare)           |

## CLI

```zsh
zpmod bundle-build --from <dir> --out <bundle.zsh> [--max <KB>]
```

`--max` is a cap on concatenated source bytes in KiB, excluding generated comments. Zero means no cap. Collection stops before the first
file that would exceed the cap and emits a truncation diagnostic. Source bytes are preserved, with a newline between files.

Hidden entries and symlinks are excluded. Directory recursion is limited to 64 levels. Names containing newline or carriage return,
unreadable trees, invalid limits, and non-regular output paths fail. The output directory must already exist. An output inside the source
tree is excluded from collection.

Publication uses a private temporary file beside the output and an atomic rename. Failed collection or copying preserves the previous
bundle; output symlinks are rejected. A fresh bundle is reused when no collected source has a newer modification time and the size cap
matches. Deletions, backdated source changes, and same-second edits are outside this mtime-based freshness contract; remove the bundle to
force regeneration.

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
