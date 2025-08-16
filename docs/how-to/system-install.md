# System-wide Installation

Install under a prefix (requires root for typical prefixes). Use the CMake helper to configure, build, and install the module:

```zsh
scripts/cmake.configure.zsh --install-system --prefix /usr/local
```

Add the installed module directory to your `module_path` (default path shown):

```zsh
module_path+=(/usr/local/lib/zsh/site-modules)
zmodload -i zpmod
```

Keep the module early in `~/.zshrc` to profile all subsequent sourcing.

Notes:

- The helper delegates to CMake; set variables like ZPMOD_ZSH_MODDIR with `-D` flags if needed.
- For local testing, `cmake --build build-cmake --target stage` installs to a staged tree under `build-cmake/stage` used by the test suite.
