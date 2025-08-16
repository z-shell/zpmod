#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

: ${TMPDIR:=/tmp}
d=${TMPDIR%/}/zpmod_rf_empty.$RANDOM
mkdir -p $d
f=$d/x
: > $f  # empty file

# Scalar: empty string
local S
zpreadfile S $f
[[ -n ${+S} ]]
[[ $S == '' ]]

# Array split: length 0
local -a A
zpreadfile -d '\n' A $f
(( ${#A} == 0 ))

exit 0
