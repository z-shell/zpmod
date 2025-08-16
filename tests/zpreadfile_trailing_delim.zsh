#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

# LF case: a\nb\n -> two elements a, b
local f d
: ${TMPDIR:=/tmp}
d=${TMPDIR%/}/zpmod_rf_trail.$RANDOM
mkdir -p $d
f=$d/x
print -n -- $'a\nb\n' > $f
local -a A
zpreadfile -d '\n' A $f
(( ${#A} == 2 ))
[[ ${A[1]} == 'a' ]]
[[ ${A[2]} == 'b' ]]

# CR case: a\rb\r -> two elements a, b
print -n -- $'a\rb\r' > $f
local -a B
zpreadfile -d '\r' B $f
(( ${#B} == 2 ))
[[ ${B[1]} == 'a' ]]
[[ ${B[2]} == 'b' ]]

exit 0
