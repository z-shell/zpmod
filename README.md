# zpmod

High-performance Zsh module that accelerates script sourcing and provides fast filesystem helpers.

- Transparent, opportunistic .zwc compilation for sourced scripts
- Builtins and subcommands for fast path stats, directory listing, and file reads
- Clean CMake build with CTest, docs, and staging helpers

## Quick start

```zsh
# Add early in ~/.zshrc
module_path+=("${HOME}/.zi/zmodules/zpmod")
zmodload -i zpmod

# After shell start, profile sourced scripts
zpmod source-study
```

## Install

Use the CMake helper script from the repo root:

```zsh
# Zi-style install (Zi not required for loading)
scripts/cmake.configure.zsh --install-zi

# Or install for current user (~/.local/lib/zsh/site-modules)
scripts/cmake.configure.zsh --install-user

# Or system-wide (prefix defaults to /usr/local)
scripts/cmake.configure.zsh --install-system --prefix /usr/local
```

Then add the install directory to `module_path` and load once per shell session.

## Docs

Full documentation lives under `docs/`:

- Tutorials: [docs/tutorials/first-use.md](docs/tutorials/first-use.md)
- How‑to Guides: [docs/how-to/README.md](docs/how-to/README.md)
- Reference: [docs/reference/README.md](docs/reference/README.md)
- Explanation: [docs/explanation/README.md](docs/explanation/README.md)

## License

See [LICENSE](LICENSE) if present in this repository.
