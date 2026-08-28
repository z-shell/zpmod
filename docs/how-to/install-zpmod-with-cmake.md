# Install zpmod with the CMake helper

This script helps you build and install the `zpmod` module and place the module artifact (e.g., `zpmod.so`) where Zsh can load it.

Common install flags:

- `--install-zi` — copy to Zi modules dir: `${ZI[ZMODULES_DIR]}/zpmod`
- `--install-user` — copy to user-local site modules: `~/.local/lib/zsh/site-modules`
- `--install-system` — system-wide install via CMake. Uses `--prefix` if set (defaults to `/usr/local`) and installs under
  `${prefix}/lib/zsh/site-modules`.

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

```zsh
module_path+=( "${ZI[ZMODULES_DIR]}/zpmod" )
zmodload -i zpmod
```

```zsh
module_path=( "$HOME/.local/lib/zsh/site-modules" $module_path )
zmodload -i zpmod
```

```zsh
module_path=( "/usr/local/lib/zsh/site-modules" $module_path )
zmodload -i zpmod
```

Tip: the script prints a ready-to-copy hint after installing; you can paste that into your `~/.zshrc`.

For `--install-zi`, an active `ZI[ZMODULES_DIR]` wins. Without a loaded Zi, the helper retains a recognized legacy `$HOME/.zi` installation;
otherwise it uses `${XDG_DATA_HOME}/zi` when the variable is absolute, or `$HOME/.local/share/zi` when it is unset, empty, or relative. It
creates no parallel installation and performs no migration. Set `ZI[ZMODULES_DIR]` explicitly before running the helper to select a custom
Zi destination.

## Build performance options

You can control two optional performance toggles at configure time:

- Link-Time Optimization (IPO/LTO): enabled by default if supported
- Native CPU tuning: opt-in, not portable across machines

Examples:

```zsh
# Enable (default) or disable LTO explicitly
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DZPMOD_ENABLE_LTO=ON
cmake --build build-cmake -j

# Enable native tuning (non-portable)
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DZPMOD_ENABLE_NATIVE=ON
cmake --build build-cmake -j
```

Notes:

- LTO may increase link time but can improve runtime performance.
- `-march=native` generates code optimized for your CPU and may not run on older/different machines.
- A zpmod binary is also tied to its operating system, architecture, and Zsh ABI. Do not reuse one shared Zi-data copy across incompatible
  environments; rebuild it or use a machine-local explicit destination.

## Notes

- The exact system module directory can vary by distro or architecture. Using `lib/zsh/site-modules` keeps third-party modules
  version-agnostic and matches this project’s CMake defaults.
- Verify the module is available:

```zsh
zmodload -L | grep zpmod || print -r -- "zpmod not loaded"
print -rl -- $module_path
```

## Platform notes

- Linux/BSD:
  - Modules typically use the `.so` suffix and live under a site path like `lib/zsh/site-modules`.
  - Some distros use multi-arch dirs (e.g., `lib64`). If your module path differs, add that directory to `module_path`.

- macOS:
  - Loadable modules may appear as `.so`, `.bundle`, or `.dylib`. This project prefers `.so` for consistency with `zmodload`.
  - If your build produced a different suffix, just add the containing directory to `module_path`; `zmodload -i zpmod` will still work.

- Windows (Cygwin/MSYS/WSL):
  - Artifacts may use `.dll`. Add the directory that contains `zpmod.dll` to `module_path` and run `zmodload -i zpmod`.

The helper script attempts to auto-detect the built artifact across these suffixes when copying/installing.
