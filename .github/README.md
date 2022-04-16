# ZPMOD

The module is a binary Zsh module (think about `zmodload` Zsh command, it's that topic) which transparently and
automatically **compiles sourced scripts**. Many plugin managers do not offer compilation of plugins, the module is
a solution to this. Even if a plugin manager does compile plugin's main script (like ZI does).

## Installation

### Without [ZI](https://github.com/z-shell/zi)

Install just the **standalone** binary which can be used with anyother plugin manager.

> **[?]**
> This script can be used with most plugin managers and [ZI](https://github.com/z-shell/zi) is not required.

```sh
sh <(curl -fsSL https://raw.githubusercontent.com/z-shell/zpmod/HEAD/build.sh)
```

This script will display what to add to `~/.zshrc` (2 lines) and show usage instructions.

### With [ZI](https://github.com/z-shell/zi)

> **[?]**
> ZI users can build the module by issuing following command instead of running above `build.sh` script.

```shell
zi module build
```

This command will compile the module and display instructions on what to add to `~/.zshrc`.

## Measuring Time of sources

Besides the compilation-feature, the module also measures **duration** of each script sourcing.
Issue `zpmod source-study` after loading the module at top of `~/.zshrc` to see a list of all sourced files with the time the
sourcing took in milliseconds on the left.
This feature allows to profile the shell startup. Also, no script can pass-through that check and you will obtain a complete list of all loaded scripts,
like if Zshell itself was investigating this. The list can be surprising.

## Debugging

To enable debug messages from the module set:

```shell
typeset -g ZI_MOD_DEBUG=1
```
