#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

local tdir=$(mktemp -d)
mkdir -p $tdir/dir1 $tdir/dir2
print -r -- 'x' > $tdir/file1
print -r -- 'y' > $tdir/file2
print -r -- '' > $tdir/.hidden

local -a A
zpdirlist A $tdir
# Should skip hidden by default
for e in $A; do
  [[ $e == .hidden ]] && { print -u2 -- "dotfile not filtered"; exit 1 }
done

local -a D F
zpdirlist -d D $tdir
zpdirlist -f F $tdir
# Dirs only
for e in $D; do
  [[ -d $tdir/$e ]] || { print -u2 -- "expected dir, got $e"; exit 1 }
done
# Files only
for e in $F; do
  [[ -f $tdir/$e ]] || { print -u2 -- "expected file, got $e"; exit 1 }
done

# Subcommand parity
local -a D2 F2
zpmod dirlist -d D2 $tdir
zpmod dirlist -f F2 $tdir
(( ${#D2} == ${#D} ))
(( ${#F2} == ${#F} ))

exit 0
