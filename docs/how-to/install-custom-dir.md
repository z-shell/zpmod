# Install to a Custom Directory

Use the CMake helper with a custom prefix or install subdir:

```zsh
# Install under a custom prefix
scripts/cmake.configure.zsh --install-system --prefix /opt

# Or stage locally, then copy the artifact where you want
scripts/cmake.configure.zsh
cp build-cmake/stage/lib/zsh/site-modules/zpmod.so /opt/zpmod/
```

Then add to `~/.zshrc`:

```zsh
module_path+=(/opt/zpmod)
zmodload -i zpmod
```

If you later update, rerun with the same `--target` path.
