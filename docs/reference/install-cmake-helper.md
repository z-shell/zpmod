# CMake Build & Install Helper

The script `scripts/cmake.configure.zsh` configures, builds, stages, optionally installs, and can package the zpmod module.

Common modes:

```zsh
# Install to Zi modules dir
scripts/cmake.configure.zsh --install-zi

# Install for current user under ~/.local/lib/zsh/site-modules
scripts/cmake.configure.zsh --install-user

# System-wide install under /usr/local (requires privileges unless prefix is writable)
scripts/cmake.configure.zsh --install-system --prefix /usr/local
```

Key flags:

- `--build-type <Release|Debug|...>`: CMAKE_BUILD_TYPE (default: Release)
- `--generator <make|ninja>`: choose build tool
- `-j, --jobs <N>`: parallel jobs
- `--stage-prefix <DIR>`: where `cmake --install` stages the module (default: build-cmake/stage)
- `--moddir <REL-PATH>`: install subdir relative to prefix (default: lib/zsh/site-modules)
- `--docs`: build API docs via Doxygen
- `--package` / `--cpack-generators`: produce archives with CPack
- `--install-zi`, `--install-user`, `--install-system`: convenience installers

After running any install mode, add the module directory to `module_path` and load once per shell session:

```zsh
module_path+=("$HOME/.local/lib/zsh/site-modules")
zmodload -i zpmod
```

For local testing, the script prints resolved paths. You can also use the staged tree:

```zsh
module_path+=("$PWD/build-cmake/stage/lib/zsh/site-modules")
zmodload -i zpmod
```
