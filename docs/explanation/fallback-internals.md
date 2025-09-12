# Fallback Internals Header

## Purpose

`zpmod_internals.h` supplies a **minimal** set of forward declarations, globals, and prototypes for zsh internals that zpmod touches when
the upstream generated headers (`zsh.mdh`, `*.epro`) are missing (e.g. slim / minimal Docker images without the zsh `-dev` package).

## Activation Rules

The fallback header is only used when:

1. `vendor/zsh/` headers are present (always) **and**
2. The generated header `zsh.mdh` is **not** found by the compiler (`__has_include("zsh.mdh")` fails) **and**
3. The build did **not** define `ZSH_MDH_INCLUDED` earlier.

If the real `zsh.mdh` is present, it wins and the fallback content is completely suppressed so we do not risk type / signature drift.

## Design Principles

- Minimal surface: declare **only** what is directly referenced by zpmod sources.
- Exact fidelity: types / qualifiers (e.g. `volatile`) must mirror upstream. If uncertain, prefer omitting the symbol until needed.
- No duplication: never re-`typedef` or redefine things already covered by `zsh.h`.
- No macro collisions: do **not** declare an extern for a name that upstream exposes as a macro sentinel (e.g. `dummy_patprog1`).
- Comment rationale: every deletion / intentional omission that previously caused CI failures must be documented inline.

## Maintenance Workflow

1. Encounter a missing symbol error in a minimal build.
2. Confirm the symbol appears in upstream generated headers (search under `vendor/zsh`).
3. Add the **smallest** correct declaration to `zpmod_internals.h` with a short comment.
4. Rebuild with both minimal (no generated headers) and full (with `zsh.mdh`) environments to ensure: (a) no redefinition warnings, (b)
   functionality unaffected.
5. Run smoke tests (`CTest: smoke`), then full suite (`CTest: all`) before merging.

## Guarding Against Macro / Extern Collisions

Historically we declared `extern Patprog dummy_patprog1;` while upstream defines:

```c
#define dummy_patprog1 ((Patprog) 1)
```

That caused a compile failure in Docker CI where macro expansion produced invalid syntax inside the `extern` declaration.

To prevent regressions a guard script (`scripts/guard_fallback_conflicts.zsh`) checks for any name that is both:

- Declared as a **variable** via `extern` in the fallback header, and
- Defined as a macro (no parameters) somewhere under `vendor/zsh/`.

Integrate it in CI (future task) to fail fast if a conflict appears.

See also:

- `docs/explanation/include-policy.md` for the enforced gateway include ordering (`zpmod.mdh` then `zpmod.pro`).
- `src/include/zpmod_vendor_shims.h` for non-invasive warning suppression that deliberately avoids modifying vendored sources.

## Adding New Declarations Checklist

- [ ] Verified the symbol is used by zpmod code.
- [ ] Verified not already available via `zsh.h` when `zsh.mdh` is present.
- [ ] Confirmed it is **not** a macro in upstream.
- [ ] Exact signature copied (types / qualifiers / return type).
- [ ] Added brief comment if non-obvious.
- [ ] Build + tests pass in both header-present and header-absent scenarios.

## Non-Goals

- Providing a stable public API to zsh internals (the fallback is implementation glue only).
- Expanding coverage to unused internals.
- Abstracting or wrapping internals beyond what is necessary for compilation.

## Future Improvements

- CI job to run the guard script automatically.
- Optional static analysis pass (clang-tidy) gated but non-blocking initially.
- Script to diff upstream generated declarations against fallback for drift detection.

---

Last updated: 2025-08-23
