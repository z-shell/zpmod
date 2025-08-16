#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

local tdir
tdir=$(mktemp -d)
local f=$tdir/zpmod_rf0_$$.bin

# Create NUL-delimited content: "a\0b\0c"
: >| $f
print -rn -- a >| $f
print -rn -- $'\0' >> $f
print -rn -- b >> $f
print -rn -- $'\0' >> $f
print -rn -- c >> $f

local -a A
zpreadfile -d $'\0' A $f
(( ${#A} == 3 ))
[[ $A[1] == a && $A[2] == b && $A[3] == c ]]

exit 0
