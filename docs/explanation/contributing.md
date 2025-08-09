# Contributing to zpmod

Thanks for your interest in improving zpmod! This page gathers developer-facing guidance and entry points.

- Developer docs overview: see [Explanation Overview](README.md)
- Coding style and comment conventions: see [Commenting guidelines](commenting.md)
- Architecture and design context: see [Architecture](architecture.md)
- Build and compilation strategy: see [Compilation & Caching Strategy](compilation-strategy.md)
- Performance and profiling design: see [Profiling](profiling.md)

## Development quickstart

- Build: use CMake from the repository root
  - Out-of-tree build directory: `build-cmake/`
  - Docs target: `cmake --build build-cmake --target docs`
- Tests: run the ztst-based suite under `tests/` via CTest wrappers
- Zsh compatibility: verify across multiple zsh versions; prefer zsh allocators (zalloc/zsfree)

## Pull requests

- Keep changes focused; add/update tests for behavior changes
- Follow the [commenting guidelines](commenting.md); prefer clear rationale in commit messages
- If you touch public-facing behavior, update relevant docs in `docs/`

## Reporting issues

Please include:

- zsh version and platform
- Reproducer (minimal zsh snippet or test)
- Expected vs. actual behavior

Thank you for contributing!
