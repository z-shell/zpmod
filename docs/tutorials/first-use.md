# Tutorial: First Use of zpmod

This tutorial gets you from zero to having zpmod loaded and verifying its two core capabilities:

1. Transparent, automatic compilation of sourced scripts (.zwc generation & usage)
2. Profiling of all sourced files during shell startup

Estimated time: 5 minutes

## 1. Install

Recommended: use the provided install script to fetch and build the module into your Zi modules tree (Zi not required):

```sh
sh <(curl -fsSL https://raw.githubusercontent.com/z-shell/zpmod/main/scripts/install.sh)
```

The script prints the lines to add to your `~/.zshrc`.

Manual clone/build (advanced users):

```sh
git clone https://github.com/z-shell/zpmod.git
cd zpmod
./scripts/install.sh --build-only --no-git # reuse existing clone
```

## 2. Load Early

Place the lines output by the installer at the very top of `~/.zshrc` (before plugin managers) so zpmod can intercept all early `source` / `.` calls:

```zsh
module_path+=("${HOME}/.zi/zmodules/zpmod")
zmodload zpmod
```

## 3. Start a New Shell

Open a new terminal (or `exec zsh`). The module will:

- Hook `.` and `source`
- Attempt to compile sourced scripts to `.zwc` if missing / stale
- Record timing for each sourced file

## 4. Inspect Profile

Run:

```zsh
zpmod source-study    # show relative paths by default
zpmod source-study -l # show full paths
```

You will see a list like:

```text
  2 ms    ~/.zshrc.d/env.zsh
  5 ms    ~/.zsh/plugins/someplugin/init.zsh
 12 ms    ~/.zsh/plugins/another/init.zsh
```

## 5. Verify Compilation

Pick a sourced script from the list, and look for a sibling `script.zwc`. If it exists and timestamps match or are newer, zpmod is successfully compiling.

## 6. Enable Debug (Optional)

For more verbose messages (compilation attempts, skips):

```zsh
typeset -g ZI_MOD_DEBUG=1
```

Reload your shell to see diagnostic warnings (e.g. skipped compilation due to permissions).

## Next Steps

- Read the [How-to Guides](../how-to/README.md) to accomplish specific tasks.
- Consult the [Reference](../reference/README.md) for builtin flags and environment variables.
