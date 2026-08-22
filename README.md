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

## Documentation

Full documentation lives under [docs/](docs/index.md), organized as tutorials, how-to guides, reference, and explanation.

## Contributing

See [docs/explanation/contributing.md](docs/explanation/contributing.md).

## License

`zpmod` is distributed under the Zsh license; see [LICENSE](LICENSE). It builds against and vendors Zsh itself (`vendor/zsh`), copyright (c)
the Zsh Development Group; see that submodule for its own licensing terms.
