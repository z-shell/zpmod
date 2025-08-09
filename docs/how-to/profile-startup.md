# Profile Shell Startup

1. Ensure zpmod loads first in `~/.zshrc`.
2. Start a new shell.
3. Run:

```zsh
zpmod source-study         # relative paths
zpmod source-study -l      # full absolute paths
```

Interpretation:

```text
  3 ms    ~/.zsh/plugins/foo/init.zsh
 12 ms    ~/.zsh/plugins/bar/bar.plugin.zsh
```

Large values highlight optimization candidates (e.g. lazy loading, precompilation). Re-run after changes to measure improvement.
