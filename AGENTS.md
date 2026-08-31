# Project Guidelines: zpmod

This project follows the organization-wide [Z-Shell Organization Guidelines](https://github.com/z-shell/.github/blob/main/AGENTS.md).

## What this repo is

`zpmod` is a compiled binary Zsh module (`zmodload`) that provides automatic `.zwc` compilation for sourced scripts and profiling tools
(`source-study`).

## Release model

**Class 2: Tag-driven versioned tool.**

- Development and production branch: `main`.
- Releases: Tagged with semver tags (e.g. `v2.0.6`).
- Reference: [org release runbook](https://github.com/z-shell/.github/blob/main/runbooks/release.md).

## Branching and commits

- All work branches from `main`, and pull requests target `main`.
- Use `feature-<id>`, `bug-<id>`, or `hotfix-<id>` branch names.
- Commit format: [Conventional Commits](https://www.conventionalcommits.org/); see
  [decision 0003](https://github.com/z-shell/.github/blob/main/decisions/0003-conventional-commits.md).
- Branch model: [decision 0019](https://github.com/z-shell/.github/blob/main/decisions/0019-trunk-on-main-default.md).
- A `Co-authored-by` trailer may credit a real human. Never credit a bot, AI agent, or automation as a co-author.

## Project layout

| Path                       | Purpose                                             |
| -------------------------- | --------------------------------------------------- |
| `src/module/`              | C source code for the Zsh dynamic module            |
| `cmake/`, `CMakeLists.txt` | CMake build configuration                           |
| `tests/`                   | Test suites for module build, loading, and features |
| `benchmarks/`              | Reproducible benchmark harness and results          |
| `docs/`                    | User documentation and installation guides          |

## Build and test

```bash
# Configure and build with CMake
cmake -B build -S .
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure
```

Validate the full suite and supported Zsh ABI boundaries before proposing releases.

## Key org cross-references

- [Z-Shell Organization Guidelines](https://github.com/z-shell/.github/blob/main/AGENTS.md)
- [PATTERNS.md](https://github.com/z-shell/.github/blob/main/PATTERNS.md)
- [GitHub issues](https://github.com/z-shell/zpmod/issues) and [pull requests](https://github.com/z-shell/zpmod/pulls)
