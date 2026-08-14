# ZPMOD

<div align="center">

[![🍎 Build (MacOS)](https://github.com/z-shell/zpmod/actions/workflows/test-macos.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/test-macos.yml)
[![🐧 Build (Linux)](https://github.com/z-shell/zpmod/actions/workflows/test-linux.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/test-linux.yml)

</div><hr />

The module is a binary Zsh module (think about `zmodload` Zsh command, it's that topic) which transparently and automatically **compiles sourced scripts**. Many plugin managers do not offer compilation of plugins, the module is a solution to this. Even if a plugin manager does compile plugin's main script (like Zi does).

## Installation

### Without [Zi](https://github.com/z-shell/zi)

#### Traditional system or package installation

The traditional build is the recommended path for system administrators and
package maintainers. It does not clone, update, or download anything during the
build:

```sh
./configure --prefix=/usr \
  --enable-cflags="-g -Wall -Wextra -O3" \
  --disable-gdbm \
  --without-tcsetpgrp
make
```

Zsh modules are versioned and may use a distribution-specific or multiarch
directory. Resolve the target using the Zsh that will load `zpmod`:

```sh
zsh_module_dir=$(zsh -fc 'print -r -- $module_path[1]')
```

For a direct system installation:

```sh
sudo make MODDIR="$zsh_module_dir" install
```

For a package build, stage files with `DESTDIR`:

```sh
pkgdir=$PWD/pkg
make DESTDIR="$pkgdir" MODDIR="$zsh_module_dir" install
```

This installs one namespaced module at
`$MODDIR/zi/zpmod.<platform-extension>`. Use the same variables to remove it:

```sh
make DESTDIR="$pkgdir" MODDIR="$zsh_module_dir" uninstall
```

`--prefix` controls Autotools defaults such as `libdir`; `DESTDIR` adds a
packaging root without changing installed paths; `MODDIR` selects the exact Zsh
module directory. Package recipes should set `MODDIR` explicitly because the
bundled build metadata cannot infer the target distribution's Zsh layout,
especially for cross builds.

Do not use `Scripts/install.sh` or `build.sh` in package builds. Those are
interactive convenience entry points and can clone or update a checkout unless
`--no-git` is supplied.

#### Convenience build

For an interactive per-user build, clone the repository and run `build.sh`:

```sh
git clone https://github.com/z-shell/zpmod.git
cd zpmod
./build.sh --no-git
```

The build script supports these options:

| Option                         | Description                                                       |
| ------------------------------ | ----------------------------------------------------------------- |
| `--target=DIR`, `--target DIR` | Install to a specific directory                                   |
| `--clean`                      | Run `make distclean` instead of `make clean`                      |
| `--quiet`, `-q`                | Suppress non-essential output                                     |
| `--verbose`, `-v`              | Show more detailed build information                              |
| `--no-git`                     | Skip git clone/pull operations                                    |
| `--force`, `-f`                | Force rebuild even if Makefile exists                             |
| `--build-only`                 | Build but don't update .zshrc                                     |
| `--cflags="..."`               | Pass custom CFLAGS to configure (default: `-g -Wall -Wextra -O3`) |
| `--branch=NAME`                | Use specific git branch (default: main)                           |
| `--zsh-path=PATH`              | Use specific Zsh executable                                       |
| `--jobs=N`, `-jN`              | Set number of parallel make jobs                                  |
| `--prefix=DIR`                 | Set convenience target root (`DIR/share/zsh/zpmod`)               |
| `--no-install`                 | Skip installation after building                                  |
| `--help`, `-h`                 | Show help message                                                 |

#### Examples

```sh
# Install to a custom directory
./build.sh --target=/opt/zsh-modules/zpmod

# Build with specific compiler optimizations
./build.sh --cflags="-O3 -march=native"

# Quiet installation with 8 parallel jobs
./build.sh --quiet --jobs=8
```

### With [Zi](https://github.com/z-shell/zi)

> **Note**
> Zi users can build the module by issuing the following command instead of running the above installation scripts.

```shell
zi module build
```

This command will compile the module and display instructions on what to add to `~/.zshrc`.

## Loading the Module

After a traditional installation, ensure the selected module directory is in
`module_path`, then load and verify the module:

```zsh
module_path+=( /usr/lib/zsh/5.9 ) # Use your resolved $zsh_module_dir.
zmodload zi/zpmod
(( ${+builtins[zpmod]} ))
zpmod -h
```

The convenience build prints its module directory. Its default can be loaded
with:

```zsh
module_path+=( "${HOME}/.zi/zmodules/zpmod/Src" )
zmodload zi/zpmod
```

## Measuring Time of Sources

Besides compilation, the module measures the **duration** of each sourced
script. Compiled files are written as adjacent `*.zwc` files when the source is
a writable regular file.

Run `zpmod source-study` after loading the module near the top of `~/.zshrc` to
list sourced files and their load times in milliseconds. This report is a
timing profile, not an indication that every listed source was compiled.

## Debugging

To enable debug messages from the module set:

```shell
typeset -g ZI_MOD_DEBUG=1
```

## System Requirements

- Zsh version 5.8.1 or newer
- GCC or compatible compiler
- Make
- Git (optional, can be skipped with `--no-git`)

## Troubleshooting

If you encounter build issues:

1. Use `--verbose` to see detailed build output
2. Check the `make.log` file in the build directory
3. Make sure your Zsh version is compatible (5.8.1+)
4. Try with `--clean` to perform a fresh build
5. Submit an issue with the error messages on the [GitHub repository](https://github.com/z-shell/zpmod/issues)
