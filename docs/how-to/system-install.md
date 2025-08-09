# System-wide Installation

Install under a prefix (requires root for typical prefixes). The installer is CMake-based and will configure, build, and install the module:

```sh
sudo ./scripts/install.sh --prefix /usr/local
```

The module path then becomes (verify actual output):

```zsh
module_path+=(/usr/local/share/zsh/zpmod)
zmodload zpmod
```

Keep the module early in `~/.zshrc` to profile all subsequent sourcing.

Notes:

- The install script delegates to CMake; set variables like ZPMOD_ZSH_MODDIR with `-D` flags if needed.
- For local testing, `cmake --build build-cmake --target stage` installs to a staged tree under `build-cmake/stage` used by the test suite.
