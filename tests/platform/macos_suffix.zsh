#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="platform/macos_suffix"

[[ $(uname) == Darwin ]] || { test_status "SKIP" "$TEST_NAME"; exit 0 }

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)

# Ensure suffix is .so (project sets this on Apple)
[[ -f $moddir/zpmod.so ]]

# Load succeeds
zmodload -i zpmod
whence -w zpmod >/dev/null

test_status "PASS" "$TEST_NAME"
exit 0
