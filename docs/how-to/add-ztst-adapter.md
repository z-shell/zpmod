# Add a minimal ztst adapter to zpmod tests

This repository primarily uses zsh test scripts with CTest. For internals-heavy checks aligned with upstream zsh conventions, we include a
tiny, dependency-free ztst-like adapter.

## Files

- `tests/ztst/ztst_mini.zsh` — lightweight adapter to define and run small ztst-style cases
- `tests/ztst/options_mapping.ztst` — sample cases for option mapping and emulate interaction
- `tests/ztst/param_hash_semantics.ztst` — sample cases for param/hash behaviors

## Running

After configuring and building:

```sh
ctest --test-dir build-cmake -L ztst --output-on-failure
```

Or run a specific test:

```sh
ctest --test-dir build-cmake -R zpmod_ztst_options_mapping --output-on-failure
```

## Scope

- No external dependencies on zsh's upstream `ztst.zsh` harness
- Exact/contains stdout checks and status assertions
- Meant to complement (not replace) the existing CLI/E2E zsh tests

## Notes

- Prefer stdout checks; stderr capture is best-effort in this minimal adapter.
- Keep cases small and focused; for complex E2E behavior, use the standard tests under `tests/`.
