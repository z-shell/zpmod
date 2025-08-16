#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

# dirlist parity
local tdir
tdir=$(mktemp -d)
mkdir -p $tdir/dirA
print -r -- fileA > $tdir/fileA
local -a A B
zpdirlist A $tdir
zpmod dirlist B $tdir
[[ ${#A} -eq ${#B} ]]

# pathstat parity (just check both produce some output)
local -a IN=( $tdir $tdir/fileA )
local -a S1 S2
zppathstat S1 IN
zpmod pathstat S2 IN
(( ${#S1} == ${#S2} ))
[[ $S1[1] == (path=*) ]]

# readfile parity
local f=$tdir/x
print -r -- 'a' > $f
print -r -- 'b' >> $f
local s1 s2
zpreadfile s1 $f
zpmod readfile s2 $f
[[ $s1 == $s2 ]]

exit 0
