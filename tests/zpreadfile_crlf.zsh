#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

local f d
: ${TMPDIR:=/tmp}
d=${TMPDIR%/}/zpmod_rf_crlf.$RANDOM
mkdir -p $d
f=$d/x
# Write CRLF lines
print -nr -- "a\r\nb\r\n" > $f

local -a A
zpreadfile -d $'\n' A $f   # split by LF
(( ${#A} == 3 ))
[[ ${A[1]} == $'a\r' ]]
[[ ${A[2]} == $'b\r' ]]
[[ -z ${A[3]} ]]

local -a B
zpreadfile -d $'\r' B $f   # split by CR
(( ${#B} == 3 ))
[[ ${B[1]} == 'a' ]]
[[ ${B[2]} == 'b' ]]
[[ -z ${B[3]} ]]

exit 0
