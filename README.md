# zpmod

⚙️ Zsh module that transparently and automatically compiles sourced scripts.

`zpmod` is a binary Zsh module (`zmodload`) that gives any Zsh setup automatic `.zwc` compilation of sourced scripts, including setups whose
plugin manager does not compile plugins itself (and setups that don't use a plugin manager at all).

## Quick start

```zsh
# Add to the top of ~/.zshrc
module_path+=("${HOME}/.zi/zmodules/zpmod")
zmodload -i zpmod

# After the shell starts, profile sourced scripts
zpmod source-study
```

## Installation

- Traditional system or package install (no `zi`, no network access during the build): see
  [docs/how-to/system-install.md](docs/how-to/system-install.md).
- Install with the CMake helper (`zi`, user, or system scope): see
  [docs/how-to/install-zpmod-with-cmake.md](docs/how-to/install-zpmod-with-cmake.md).
- Install to a custom directory: see [docs/how-to/install-custom-dir.md](docs/how-to/install-custom-dir.md).

## Compatibility

The minimum supported Zsh release is 5.8.1. Prebuilt module compatibility is limited to the platform, architecture, and Zsh combinations
exercised by the release and compatibility workflows. See the [compatibility reference](docs/reference/compatibility.md) for the package ABI
policy and verification coverage.

## Documentation

Full documentation lives under [docs/](docs/index.md), organized as tutorials, how-to guides, reference, and explanation.

## Contributing

See [docs/explanation/contributing.md](docs/explanation/contributing.md).

## License

zpmod contains file-specific licensing. Project-owned module sources marked `SPDX-License-Identifier: MIT` use the
[MIT license](LICENSES/MIT.txt); files without a more specific notice use the [Zsh license](LICENSE). See [NOTICE](NOTICE) for the
package-level scope. The vendored Zsh source remains copyright the Zsh Development Group and retains its own licensing terms.
