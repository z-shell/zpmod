#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

local tdir=${TMPDIR:-$(mktemp -d)}
local f=$tdir/zpmod_rf_$$.txt
print -r -- 'a' > $f
print -r -- 'b' >> $f

local s
zpreadfile s $f
[[ $s == $'a\nb\n' ]]

local -a A
zpreadfile -d $'\n' A $f
(( ${#A} == 2 ))
[[ $A[1] == 'a' && $A[2] == 'b' ]]

exit 0
