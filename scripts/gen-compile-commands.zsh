#!/usr/bin/env zsh
# Regenerate compile_commands.json before running trunk checks/format.
# This uses the repo's standardized CMake tasks when available for reproducibility.

set -euo pipefail

repo_root=${0:A:h:h}
cd "$repo_root"

# Prefer VS Code task for reproducibility, else fall back to direct CMake calls.
# The trunk hook environment may not have VS Code tasks available, so we run the
# direct CMake configure/build to ensure compile database is produced.

# If build dir doesn't exist or CMakeCache changed, configure.
BUILD_DIR="${repo_root}/build-cmake"

if [[ ! -d $BUILD_DIR ]] || [[ ! -f $BUILD_DIR/CMakeCache.txt ]] || [[ ! -f $BUILD_DIR/compile_commands.json ]]; then
  print -- "[zpm] Configuring CMake (Release) to generate compile_commands.json…"
  cmake -S "$repo_root" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release >/dev/null
fi

# Build the copy-compile-commands target if available, else perform a no-op build
# to trigger compile_commands.json generation and copy.
print -- "[zpm] Ensuring compile_commands.json is up to date…"
if cmake --build "$BUILD_DIR" --target copy-compile-commands >/dev/null 2>&1; then
  :
else
  cmake --build "$BUILD_DIR" -j 2 >/dev/null
  # Some setups generate compile_commands.json in build dir only; copy to root if needed.
  if [[ -f "$BUILD_DIR/compile_commands.json" ]]; then
    cp -f "$BUILD_DIR/compile_commands.json" "$repo_root/compile_commands.json"
  fi
fi

print -- "[zpm] compile_commands.json ready."
