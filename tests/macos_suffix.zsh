#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

[[ $(uname) == Darwin ]] || exit 0

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)

# Ensure suffix is .so (project sets this on Apple)
[[ -f $moddir/zpmod.so ]]

# Load succeeds
zmodload -i zpmod
whence -w zpmod >/dev/null

exit 0
