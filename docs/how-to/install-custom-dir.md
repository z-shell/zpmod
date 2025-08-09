# Install to a Custom Directory

Use the installer with `--target` (CMake-driven under the hood):

```sh
./scripts/install.sh --target /opt/zpmod
```

Then add to `~/.zshrc`:

```zsh
module_path+=(/opt/zpmod/Src)
zmodload zpmod
```

If you later update, rerun with the same `--target` path.
