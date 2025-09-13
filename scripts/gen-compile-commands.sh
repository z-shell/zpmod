#!/bin/sh
# Regenerate compile_commands.json before running trunk checks/format.
# POSIX sh-compatible (no bash/zsh dependency) for CI and Trunk sandboxes.

set -eu

# Resolve repo root relative to this script location
SCRIPT_PATH="$0"
# Handle cases where $0 may be relative
SCRIPT_DIR=$(
  CDPATH=
  cd "$(dirname -- "${SCRIPT_PATH}")" >/dev/null 2>&1 && pwd -P
)
REPO_ROOT=$(
  CDPATH=
  cd "${SCRIPT_DIR}/.." >/dev/null 2>&1 && pwd -P
)
cd "${REPO_ROOT}"

BUILD_DIR="${REPO_ROOT}/build-cmake"

# Configure if needed
if [[ ! -d "${BUILD_DIR}" ]] || [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]] || [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
  echo "[zpm] Configuring CMake (Release) to generate compile_commands.json…"
  cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release >/dev/null
fi

# Build the copy-compile-commands target if available; else do a minimal build
echo "[zpm] Ensuring compile_commands.json is up to date…"
if cmake --build "${BUILD_DIR}" --target copy-compile-commands >/dev/null 2>&1; then
  :
else
  cmake --build "${BUILD_DIR}" -j 2 >/dev/null || cmake --build "${BUILD_DIR}" >/dev/null
  if [[ -f "${BUILD_DIR}/compile_commands.json" ]]; then
    cp -f "${BUILD_DIR}/compile_commands.json" "${REPO_ROOT}/compile_commands.json"
  fi
fi

echo "[zpm] compile_commands.json ready."
