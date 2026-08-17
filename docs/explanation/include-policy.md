# Include Gateway & Ordering Policy

This document codifies the enforced header inclusion pattern for all zpmod C translation units.

## Goals

- Provide a single consistent entry point for vendored zsh headers + fallbacks.
- Avoid partial visibility / missing typedef issues that previously caused build failures when `zsh.mdh` was absent.
- Prevent accidental direct inclusion of upstream generated headers (`zsh.mdh`, `*.epro`).
- Keep system headers after the gateway so feature macros from zsh headers are honored.

## Canonical Pattern

Every `*.c` file under `src/` must start (within the first ~60 lines) with the gateway pair in this exact order:

```c
#include "zpmod.mdh"
#include "zpmod.pro"
```

Optional immediately-after includes:

```c
#include "zpmod_vendor_shims.h"   /* local shims (warnings, portability) */
```

Then project headers (`zpmod_*.h`) and finally system / libc headers:

```c
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
```

## Rationale

1. `zpmod.mdh` decides whether to use real generated zsh exports or minimal fallbacks (`zpmod_internals.h`).
2. `zpmod.pro` aggregates only the prototypes actually needed when generated `.epro` files are missing.
3. System headers come later so that configuration macros from zsh headers (e.g. feature/ABI guards) are in effect.
4. The shim header houses benign warning suppressions without mutating vendored sources.

## Enforcement Script

`scripts/check_include_order.zsh` validates:

- No direct `#include "zsh.mdh"` in module sources.
- Both gateway headers appear before any `<...>` system include.
- No duplicate consecutive gateway includes.

Run manually:

```sh
zsh scripts/check_include_order.zsh
```

CMake target:

```sh
cmake --build build-cmake --target include-order-check
```

## FAQ

### Why not auto-fix ordering?

Keeping enforcement read-only avoids unexpected churn; intentional edits remain explicit in commits.

### Can I include additional `.epro` files directly?

No. If a new symbol is required add its `.epro` to `zpmod_imports.h` (guarded by `__has_include`) so fallback builds remain minimal.

### What about analysis / stub builds?

When `ZPMOD_ANALYSIS` is set the analysis stubs are used instead of the normal gateway; this path still respects the ordering discipline.

## Future Enhancements

- Integrate the checker in CI as a required step.
- Add a `--fix` mode (optional) for bulk reformatting when large refactors land.

\_Last updated: **2025-08-24**
