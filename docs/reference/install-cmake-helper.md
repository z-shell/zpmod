# CMake Build & Install Helper

The script `scripts/cmake.configure.zsh` configures, builds, stages, installs, optionally packages, and can run tests for the zpmod module.

## Quick modes

```zsh
# Install to Zi modules dir
scripts/cmake.configure.zsh --install-zi

# Install for current user under ~/.local/lib/zsh/site-modules
scripts/cmake.configure.zsh --install-user

# System-wide install under /usr/local (requires privileges unless prefix is writable)
scripts/cmake.configure.zsh --install-system --prefix /usr/local
```

## All options (from `--help`)

- `--init-submodule` / `--no-submodule` – initialize/skip `vendor/zsh` submodule
- `--vendor-build` / `--no-vendor-build` – force/skip building `vendor/zsh`
- `--generator <make|ninja>` – CMake generator (default: make)
- `--build-type <TYPE>` – CMAKE_BUILD_TYPE (default: Release)
- `-j, --jobs <N>` – parallel jobs for make/ninja (default: auto)
- `--reconfigure` – drop CMake cache and reconfigure
- `--clean` – remove `build-cmake/` before configuring
- `--prefix <DIR>` – destination for `cmake --install`
- `--stage-prefix <DIR>` – staging prefix (default: `build-cmake/stage`)
- `--moddir <REL-PATH>` – module install subdir relative to prefix (default: `lib/zsh/site-modules`)
- `--install-zi` – copy module into `${ZI[ZMODULES_DIR]}/zpmod`
- `--install-user` – copy module into `~/.local/lib/zsh/site-modules`
- `--install-system` – run `cmake --install` to system prefix
- `--package` – build a package with CPack (default generator TGZ)
- `--cpack-generators <LIST>` – generators for CPack (e.g. `TGZ;TXZ;DEB;RPM`)
- `--docs` – build API docs via Doxygen (`docs` target)
- `--test` – run the CMake `smoke` target after build/stage
- `--ctest` – run the full CTest suite after build/stage
- `--ctest-label <LABEL>` – filter tests by label (repeatable; OR’ed)
- `--ctest-regex <REGEX>` – run tests matching regex (`-R`)
- `--ctest-jobs <N>` – parallel CTest jobs (default: `--jobs` value)
- `--ctest-color` – set `ZPMOD_TEST_COLOR=1` for colored test output
- `--ctest-debug` – set `ZPMOD_TEST_DEBUG=1` for verbose helper logs
- `--verbose` – verbose CMake/build output
- `-h, --help` – show usage

Notes:

- If `vendor/zsh/config.h` is missing and vendor build isn't disabled, the script builds vendored zsh to generate headers/prototypes required by zpmod.
- If `vendor/zsh/Src/zsh` exists, it’s used for the smoke and test runs (you can override with `-DZSH_EXECUTABLE=...` at configure time).

## Local load & testing

After any install mode, add the module directory to `module_path` and load once per shell session:

```zsh
module_path+=("$HOME/.local/lib/zsh/site-modules")
zmodload -i zpmod
```

For local testing, you can use the staged tree directly:

```zsh
module_path+=("$PWD/build-cmake/stage/lib/zsh/site-modules")
zmodload -i zpmod
```

Expected staged artifact layout:

- Shared object at: `build-cmake/stage/lib/zsh/site-modules/zpmod.so` (platform suffix may vary)
- No nested `zsh/` subdirectory should exist under `site-modules/`.

Note: if a stray nested path like `build-cmake/stage/lib/zsh/site-modules/zsh/zpmod.so` appears (often from manual debugging), the helper performs a defensive cleanup to remove that symlink and fold the directory if empty, to keep module discovery unambiguous.
