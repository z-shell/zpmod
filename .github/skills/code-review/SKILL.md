---
name: code-review
description: Review zpmod pull requests, branches, commits, and local changes for correctness, regressions, security, portability, and missing tests. Use when asked for a code review, PR review, diff review, or pre-merge check. Report only concrete defects introduced by the change, not style preferences or praise.
license: See repository LICENSE
---

# zpmod Code Review

Perform a read-only, evidence-based review. Do not modify files, create commits,
or broaden the task into implementation unless the user asks separately.

## Establish the Review Scope

1. Read the request, issue, or pull request description to understand the
   intended behavior.
2. Determine the correct comparison:
   - For a pull request or branch, compare the merge base of the actual base
     branch with the review head.
   - For local work, inspect staged, unstaged, and untracked files.
   - For named commits or paths, honor the requested range exactly.
3. Inspect the diff and then read the complete changed functions, their callers,
   relevant tests, and configuration. Use history or blame when intent is
   unclear.
4. Review issues introduced or exposed by the change. Do not report unrelated
   pre-existing problems.

Useful local commands include:

```sh
git status --short
git diff --stat
git diff
git diff --cached
git diff --check
```

Do not assume these commands include untracked files; open those files
explicitly.

## Review Priorities

### Native Zsh Module

For changes under `Src/zi/` or related Zsh internals, verify:

- The overloaded `.` and `source` builtins preserve Zsh path lookup, positional
  parameters, `argzero`, options, traps, error flags, return status, and shell
  state on every exit path.
- Source compilation is limited to appropriate regular files. Missing, stale,
  unreadable, unwritable, or invalid `.zwc` files fall back to normal sourcing
  without changing observable shell behavior.
- File descriptors, signal queues, heaps, hash nodes, and module-owned memory
  are released with the matching Zsh allocation API on success and failure.
- `setup_`, `cleanup_`, and `finish_` leave no partial hooks or dangling state.
  Repeated module load and unload remains safe.
- String lengths, timestamps, path construction, and casts cannot overflow or
  access invalid memory for realistic inputs.
- Changes remain compatible with Zsh 5.8.1 and the Zsh module ABI used on both
  Linux and macOS.

Most files directly under `Src/` are synchronized from upstream Zsh by
`Scripts/copy_from_zsh_src.zsh`. For changes there, confirm the upstream
provenance or an intentional zpmod-specific divergence. Review only the changed
behavior, not untouched upstream code.

### Installer and Build

For `build.sh`, `Scripts/`, configure files, and makefiles, verify:

- Files with a `#!/usr/bin/env sh` shebang remain POSIX `sh`; Zsh-only syntax is
  confined to Zsh scripts.
- User-controlled paths, branches, flags, and environment values are quoted and
  validated before use.
- Cleanup, clone, pull, build, and install operations cannot affect a directory
  outside the resolved target. Destructive commands require an unambiguous,
  validated path.
- `--no-git`, `--target`, `--prefix`, `--build-only`, `--no-install`, clean,
  force, quiet, verbose, and job-count behavior remains coherent and
  idempotent.
- Tool detection and commands work with the utilities available on Linux and
  macOS. Do not assume GNU-only flags without an existing portability guard.
- `build.sh` and `Scripts/install.sh` stay synchronized when they represent the
  same entry point.
- Changes to `configure.ac` are reflected in generated configure artifacts when
  required, without committing unrelated generated churn.

### Tests, CI, and Documentation

- Behavioral fixes and new features exercise both success and relevant failure
  paths. Prefer a focused regression in `Test/ci-module.zsh` or the existing
  Zsh test harness.
- Build changes preserve standalone and Zi-managed installation.
- CI changes retain least privilege, pinned actions, bounded timeouts, and the
  minimum supported Zsh 5.8.1 coverage.
- User-facing flags, requirements, output, and module commands stay consistent
  across implementation, help text, tests, and `README.md`.

## Validation

Run the smallest existing checks that cover the changed behavior:

- For POSIX shell changes, run `shellcheck` and `sh -n` on the changed scripts.
- For Zsh changes, run `zsh -n` and the compile check used by
  `.github/workflows/test-linux.yml`.
- For native or build changes, reproduce the isolated standalone build and run
  `Test/ci-module.zsh` as defined in the Linux or macOS workflow.
- Run focused `make TESTNUM=... check` tests when a matching Zsh test exists.

Do not mutate the review worktree merely to validate a change; use an isolated
temporary copy for build commands that generate files. Never claim a check ran
when it did not. Passing tests do not override a concrete defect in the diff.

## Findings

Report only findings that are actionable, reproducible from the changed code,
and likely to matter. Exclude formatting preferences, speculative concerns,
general hardening suggestions, praise, and summaries of the patch.

Order findings by severity:

- **P0**: immediate, broadly harmful breakage or critical vulnerability.
- **P1**: likely crash, data loss, security issue, or major regression.
- **P2**: concrete correctness, compatibility, or reliability defect.
- **P3**: low-impact defect; include only when the user requests an exhaustive
  review.

Use this format for each finding:

```text
[P1] Short imperative title
path/to/file:line

Explain the triggering scenario, the observable impact, and why the changed
code causes it. State a practical correction without rewriting the patch.
```

Cite the smallest changed line range that demonstrates the issue. Merge
duplicate findings with the same root cause. If no qualifying issues remain,
state: `No actionable findings.`
