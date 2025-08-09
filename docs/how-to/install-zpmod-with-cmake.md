# Install zpmod with the CMake helper

This script helps you build and install the `zpmod` module and place `zpmod.so` where Zsh can load it.

Common install flags:

- `--install-zi` — copy to Zi modules dir: `${ZI[ZMODULES_DIR]}/zpmod`
- `--install-user` — copy to user-local site modules: `~/.local/lib/zsh/site-modules`
- `--install-system` — system-wide install via CMake. Uses `--prefix` if set (defaults to `/usr/local`) and installs under `${prefix}/lib/zsh/site-modules`.

## Quick use

```zsh
# From the repo root
scripts/cmake.configure.zsh --install-zi
# or
scripts/cmake.configure.zsh --install-user
# or
scripts/cmake.configure.zsh --install-system --prefix /usr/local
```

## Load instructions

After installation, add the destination directory to `module_path` and load the module once per shell session:

- Zi install (recommended if you use Zi):

```zsh
module_path+=( "${ZI[ZMODULES_DIR]}/zpmod" )
zmodload -i zpmod
```

- User-level install:

```zsh
module_path=( "$HOME/.local/lib/zsh/site-modules" $module_path )
zmodload -i zpmod
```

- System-wide install (default prefix shown):

```zsh
module_path=( "/usr/local/lib/zsh/site-modules" $module_path )
zmodload -i zpmod
```

Tip: the script prints a ready-to-copy hint after installing; you can paste that into your `~/.zshrc`.

## Notes

- The exact system module directory can vary by distro or architecture. Using `lib/zsh/site-modules` keeps third-party modules version-agnostic and matches this project’s CMake defaults.
- Verify the module is available:

```zsh
zmodload -L | grep zpmod || print -r -- "zpmod not loaded"
print -rl -- $module_path
```
