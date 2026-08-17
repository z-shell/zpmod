# Force a Rebuild

To force a rebuild without cleaning:

```zsh
scripts/cmake.configure.zsh --reconfigure
```

To perform a full clean (remove configuration artifacts) and rebuild:

```zsh
scripts/cmake.configure.zsh --clean
```

For verbose compiler output:

```zsh
scripts/cmake.configure.zsh --verbose
```
