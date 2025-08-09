#!/usr/bin/env zsh
# Minimal smoke test: ensure zpmod loads from the staged module_path.

set -euo pipefail
emulate -L zsh

: ${ZPMOD_STAGE_MODULE_DIR:?ZPMOD_STAGE_MODULE_DIR is required}

# Prepend staged module dir to module_path
module_path=("$ZPMOD_STAGE_MODULE_DIR" $module_path)
print -r -- "module_path: $module_path"

# Quick sanity: module file should exist in staged dir
if [[ ! -e "$ZPMOD_STAGE_MODULE_DIR/zpmod.so" && ! -e "$ZPMOD_STAGE_MODULE_DIR/zpmod.bundle" && ! -e "$ZPMOD_STAGE_MODULE_DIR/zpmod.dylib" && ! -e "$ZPMOD_STAGE_MODULE_DIR/zpmod.dll" ]]; then
  print -ru2 -- "zpmod shared object not found in $ZPMOD_STAGE_MODULE_DIR"
  ls -al "$ZPMOD_STAGE_MODULE_DIR" 2>/dev/null || true
  exit 1
fi

# Load module
if ! zmodload -i zpmod 2>err.txt; then
  print -ru2 -- "zmodload failed:"; cat err.txt >&2; rm -f err.txt; exit 1
fi
rm -f err.txt

# Ensure builtins are enabled (handle feature-gating)
zmodload -F zpmod b:zpmod b:custom_dot 2>/dev/null || true

# Verify loaded by checking builtin presence
if ! whence -w zpmod | grep -q "builtin"; then
  print -ru2 -- "zpmod builtin not found after zmodload"
  print -ru2 -- "Loaded modules:"; zmodload -L 2>/dev/null || true
  exit 1
fi

print -r -- "zpmod smoke OK"
