#!/usr/bin/env zsh
# Verify that the 'zpmod' builtin is available after loading the module

set -euo pipefail
emulate -L zsh

: ${ZPMOD_STAGE_MODULE_DIR:?ZPMOD_STAGE_MODULE_DIR is required}
module_path=("$ZPMOD_STAGE_MODULE_DIR" $module_path)

zmodload -i zpmod

# 'whence -w' should show the builtin and its type
if ! whence -w zpmod | grep -q "builtin"; then
  print -ru2 -- "zpmod builtin not found"
  exit 1
fi

# Optionally check help flag works
zpmod -h >/dev/null 2>&1 || true

print -r -- "zpmod builtin present"
