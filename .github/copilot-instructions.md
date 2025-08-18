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
- Knowledge graph:
  - Read first; write only stable decisions (architecture/patterns/bug solutions)
  - Link with `depends_on`, `implements`, `tested_by`; avoid duplicates
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

## 🎯 ESSENTIAL QUICK REFERENCE

### Primary Workflow (Always Follow)

1.  **Start**: `#mcp_memory_search_nodes` - Check existing knowledge graph first.
2.  **Explore Graph**: `#mcp_memory_read_graph` - Get full context of the project knowledge.
3.  **Complex Tasks**: `#mcp_sequentialthi_sequentialthinking` - Break down problems systematically.
4.  **Documentation**: `#mcp_context7_resolve-library-id` → `#mcp_context7_get-library-docs`
5.  **Current Info**: `#vscode-websearchforcopilot_webSearch` - Research latest information.
6.  **Record Decisions**: `#mcp_memory_add_observations` - Save solutions to existing entities.
7.  **Create Entities**: `#mcp_memory_create_entities` - Add new components to knowledge graph.
8.  **Link Knowledge**: `#mcp_memory_create_relations` - Connect related entities.

## 🧠 MEMORY MANAGEMENT (Knowledge Graph)

### Entity Types to Create (`#mcp_memory_create_entities`)

- `project_component`: Major system components (parsers, allocators, commands).
- `architecture_decision`: Important design choices and rationale.
- `implementation_pattern`: Reusable code patterns and best practices.
- `bug_solution`: Resolved issues with solution approaches.
- `zpmod_layer`: Layer-specific components and interfaces.
- `code_analysis`: Findings from code examination and review.
- `refactoring_task`: Specific refactoring activities and outcomes.

### Essential Relations (`#mcp_memory_create_relations`)

- `depends_on`: Component dependencies and layer relationships.
- `implements`: Pattern implementations and interface realizations.
- `resolves`: Solutions to specific problems or requirements.
- `tested_by`: Links between components and their test files.
- `refines`: Improvements or extensions to existing components.
- `replaces`: Components that supersede older implementations.
- `has_issue`: Components with known problems that need resolution.

### Knowledge Graph Tools

- **Search**: `#mcp_memory_search_nodes` - Find entities by name, type, or content.
- **View Full Graph**: `#mcp_memory_read_graph` - Get complete context.
- **Open Specific**: `#mcp_memory_open_nodes` - View details of named entities.
- **Create**: `#mcp_memory_create_entities` - Add new knowledge components.
- **Link**: `#mcp_memory_create_relations` - Connect related knowledge.
- **Update**: `#mcp_memory_add_observations` - Add insights to existing entities.
- **Clean**: `#mcp_memory_delete_entities` / `#mcp_memory_delete_relations` / `#mcp_memory_delete_observations` - Manage knowledge graph.

## 🧮 PROBLEM-SOLVING APPROACH

### Sequential Thinking Tool (`#mcp_sequentialthi_sequentialthinking`)

For complex tasks, use the sequential thinking tool to break down problems step by step:

- **When to Use**:
  - Architectural decisions that need careful consideration
  - Bug investigations requiring multiple analysis steps
  - Refactoring plans with interdependent changes
  - Feature implementations with complex requirements

- **Key Parameters**:
  - `thought`: Current thinking step (analysis, revision, realization)
  - `thoughtNumber`: Track progress through the problem
  - `totalThoughts`: Estimated steps needed (can be adjusted)
  - `nextThoughtNeeded`: Continue problem-solving when true
  - `isRevision`: Mark when revising previous thinking

- **Process Example**:
  1. Frame the problem clearly
  2. Break into sub-problems
  3. Consider alternative approaches
  4. Analyze trade-offs
  5. Develop solution strategy
  6. Verify against requirements
  7. Finalize implementation plan

This module requires deep zsh internals knowledge - always verify changes against multiple zsh versions and test thoroughly with the ztst
framework.
