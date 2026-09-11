# Copilot instructions for zpmod

## Overview

### Purpose & scope

Work efficiently in this repo with minimal tool calls and small, focused edits. Favor local context, reproducible tasks, and zpmod’s
established patterns.

### Architecture quick brief

- Binary zsh module with layers:
  - `src/core/*` core logic (e.g., `source.c` for source-study)
  - `src/module/*` module glue and static builtin table (`module.c`)
  - `src/include/*` public cross-unit headers (`zpmod_*.h`)
  - `src/compat/*` shims (stable option mapping in `options.c`, `sigcount.h`)
  - `src/completion/_zpmod` completion script
- Invariants:
  - Builtins are registered statically; features via `features_/enables_`; `setup_` installs overrides, `finish_` restores them
  - Use zsh allocators (zalloc/zsfree); free with exact length when provided
  - Map options via stable enum (`zp_conv_opt`) from `zpmod_compat.h`/`compat/options.c`
  - Prefer vendored zsh headers; out-of-tree stubs live in `src/module/zpmod.mdh/.pro`
  - Version strings via CMake (e.g., `ZPMOD_VERSION_STR`)

### Build & test workflow (VS Code tasks)

- Configure only when cache is missing or CMake files changed: “CMake: configure”
- Build after edits: “CMake: build”
- Tests:
  - Quick: “CTest: smoke” (no behavior changes)
  - Full: “CTest: all” (behavior changes or before PR summary)
- Tests are zsh scripts under `tests/*` suites; use `tests/test_helpers.zsh`. Module path via `ZPMOD_STAGE_MODULE_DIR`.

### Tool usage (gating)

- Prefer workspace tasks over ad‑hoc shells for reproducibility
- Search/read:
  - Use semantic search for symbols; grep for exact strings
  - Read larger file chunks; batch independent reads; avoid re-reading provided context
- Docs/web:
  - Prefer local docs and vendored headers; use library docs only for integrations/upgrades
  - Use web for zsh-internals edge cases; keep summaries brief
- PR state:
  - Query active PR for status or change summary before pushing related work

### Quality gates (green-before-done)

- Build: run “CMake: build” (PASS/FAIL)
- Tests: “CTest: smoke” or “CTest: all” (PASS/FAIL)
- Docs/tests: update when behavior changes
- Patch hygiene: minimal diffs, no unrelated formatting, checkpoint after ~3–5 tool calls

### Testing & docs policy

- Add or update zsh tests in the appropriate suite when changing behavior
- Prefer smoke unless behavior changed; run full suite before PR summary
- Keep architecture docs in `docs/explanation/*`; CLI/reference in `docs/reference/*` (note `source-study -l` behavior)

### Key files & examples

- `src/module/module.c`: static builtin table and hooks (setup*/finish*/features*/enables*)
- `src/core/source.c`: source-study and source overrides
- `src/compat/options.c` + `src/include/zpmod_compat.h`: stable option mapping (`zp_conv_opt`)
- `src/compat/sigcount.h`: SIGCOUNT shim; prefer vendored headers under `vendor/zsh`
- `src/include/zpmod_*.h`: public APIs; avoid leaking internal zsh headers
- `tests/core/source_study.zsh`: formatting and path verbosity (`-l`) checks

## Optional tool integrations

Follow the organization
[optional integration guidance](https://github.com/z-shell/.github/blob/main/.github/instructions/mcp-plugins.instructions.md). Discover
tools in the current runtime before use. Use repository files, local search, vendored headers, and official documentation when an
integration is unavailable. No MCP server or external reasoning tool is required to work here.

Keep external services read-only unless the maintainer explicitly authorizes writes, including knowledge-graph updates. GitHub issues and
pull requests own active work; record durable guidance in its canonical repository artifact.

This module requires deep zsh internals knowledge - always verify changes against multiple zsh versions and test thoroughly with the ztst
framework.
