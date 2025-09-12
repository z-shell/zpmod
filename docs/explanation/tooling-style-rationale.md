# Tooling & Style Rationale

This document captures the intent behind key linter / formatter configuration so future adjustments are deliberate.

## clang-tidy

Configured rule families: `clang-analyzer-*`, `bugprone-*`, `misc-*`, `portability-*`, `readability-*`.

Intentionally excluded (for now): broader guideline suites (`cppcoreguidelines`, `google`, `modernize`, `performance`) because the project:

1. Targets C (with some zsh internal patterns) and avoids C++ abstractions those checks assume.
2. Wraps many zsh macros that confuse lifetime / ownership heuristics.
3. Prioritizes signal-safe, allocation‑aware code paths over stylistic rewrites.

### Identifier Length

We allow single character loop and scratch identifiers. Explicit allow‑list prevents churn while keeping accidental unclear names rare.

### Cognitive Complexity Threshold (60)

Lowered from 100 to gently surface refactor opportunities without flooding reports. Large dispatcher functions (`cmd_fpath_index`,
`custom_source`) are whitelisted until they are naturally decomposed.

### Header Filter

`HeaderFilterRegex` narrowed to `^(src|tests)/` to avoid analyzing vendored `zsh/` headers and build artefacts when running `clang-tidy`
directly.

## clang-format

Column limit 140 matches markdown, YAML, and prose wrap settings; avoids excessive wrapping of nested zsh macro invocations. Linux brace
style mirrors upstream zsh for easier diff mental mapping.

## ShellCheck

`enable=all` with targeted disables for zsh‑specific idioms (e.g., arrays, glob qualifiers, `emulate -L zsh`). Re‑enable candidates should
be evaluated case‑by‑case during script edits; see `.shellcheckrc`.

## Markdown / Prettier / YAML

Unified 140 width for consistency; long reference links and command lines stay readable. Markdown rule relaxations permit bare links in
commentary sections.

## Future Adjustments

Short list of potential future tightening once refactors land:

- Drop complexity threshold toward 40.
- Re‑evaluate enabling selected `performance-*` checks (non‑intrusive ones like move/return warnings likely still noisy in C context—skip
  until C++ needed).
- Consider a focused allow‑list for `bugprone-*` if false positives grow.

## Decision Tracking

Changes here should reference commits and (if applicable) issues or ADRs. Keep this file concise; deep rationale belongs in ADRs under
`docs/explanation/`.

---

Maintainers: update when materially changing any linter / formatter behavior.
