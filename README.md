# ZPMOD

<div align="center">

[![🍎 Build (MacOS)](https://github.com/z-shell/zpmod/actions/workflows/test-macos.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/test-macos.yml)
[![🐧 Build (Linux)](https://github.com/z-shell/zpmod/actions/workflows/test-linux.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/test-linux.yml)

</div><hr />

The module is a binary Zsh module (think about `zmodload` Zsh command, it's that topic) which transparently and automatically **compiles sourced scripts**. Many plugin managers do not offer compilation of plugins, the module is a solution to this. Even if a plugin manager does compile plugin's main script (like Zi does).

## Installation

### Without [Zi](https://github.com/z-shell/zi)

#### Quick Install (Recommended)

Install just the **standalone** binary which can be used with any other plugin manager.

> **Note**
> This script can be used with most plugin managers and [Zi](https://github.com/z-shell/zi) is not required.

```sh
sh <(curl -fsSL https://raw.githubusercontent.com/z-shell/zpmod/main/Scripts/install.sh)
```

This script will display what to add to `~/.zshrc` (2 lines) and show usage instructions.

#### Manual Install with Advanced Options

You can also clone the repository and use the included build.sh script with various configuration options:

```sh
git clone https://github.com/z-shell/zpmod.git
cd zpmod
./build.sh [OPTIONS]
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
| `--prefix=DIR`                 | Set installation prefix (for system installs)                     |
| `--no-install`                 | Skip installation after building                                  |
| `--help`, `-h`                 | Show help message                                                 |

#### Examples

```sh
# Install to a custom directory
./build.sh --target=/opt/zsh-modules/zpmod

# Build with specific compiler optimizations
./build.sh --cflags="-O3 -march=native"

# System installation
sudo ./build.sh --prefix=/usr/local

# Quiet installation with 8 parallel jobs
./build.sh --quiet --jobs=8

# Development build from a specific branch
./build.sh --branch=develop --verbose
```

### With [Zi](https://github.com/z-shell/zi)

> **Note**
> Zi users can build the module by issuing the following command instead of running the above installation scripts.

```shell
zi module build
```

This command will compile the module and display instructions on what to add to `~/.zshrc`.

## Loading the Module

After installation, add these lines at the top of your `~/.zshrc`:

```zsh
# Adjust the path if you installed to a custom location
module_path+=( "${HOME}/.zi/zmodules/zpmod/Src" )
zmodload zi/zpmod
```

## Measuring Time of Sources

Besides the compilation-feature, the module also measures **duration** of each script sourcing.
Issue `zpmod source-study` after loading the module at top of `~/.zshrc` to see a list of all sourced files with the time the
sourcing took in milliseconds on the left.
This feature allows you to profile the shell startup. Also, no script can pass through that check and you will obtain a complete list of all loaded scripts,
like if Zshell itself was investigating this. The list can be surprising.

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
