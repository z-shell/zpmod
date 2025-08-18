# src layout

This directory contains the zpmod module sources organized by layer:

- core: cross-cutting logic and overrides (e.g., `source.c`, `fs.c`, `utils.c`, `emoji.c`)
- builtins: builtin entrypoints (e.g., `zpmod_builtin.c`, `readarray.c`, `fs_builtins.c`)
- compat: compatibility shims and stable mappings across zsh versions (e.g., `options.c`, `sigcount.h`)
- include: public cross-unit headers (`zpmod_*.h`) — do not include internal zsh headers here
- module: zsh module glue (static builtin table in `module.c`, `zpmod.mdh/.pro` stubs and imports)
- completion: zsh completion script `_zpmod`

## Maintenance rules

- Builtins are registered statically in `module/module.c`; features via `features_/enables_`; `setup_` installs overrides, `finish_` restores.
- Use zsh allocators (`zalloc`, `zfree`, `zsfree`); when length is known, free with the exact length.
- Map options via the stable enum using `compat/options.c` and `include/zpmod_compat.h` (`zp_conv_opt`).
- Prefer vendored zsh headers under `vendor/zsh`; keep public APIs within `include/zpmod_*.h` to avoid leaking internal headers between units.

## Module imports

- `module/zpmod.mdh` prefers `zsh.mdh` when available. If not, it includes `module/zpmod_imports.h`.
- `module/zpmod_imports.h` aggregates only the required zsh `*.epro` headers.
  - Add a new `*.epro` include only when you introduce a new zsh symbol and the build reports a missing prototype/export.

## Build/test

- Use VS Code tasks: Configure (when needed) → Build → CTest (smoke for quick, all before PRs).
- Tests live under `tests/*` and run via CTest; module loaded from the staged prefix.
