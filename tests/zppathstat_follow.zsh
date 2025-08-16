#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

local tdir=$(mktemp -d)
local tgt=$tdir/real
local lnk=$tdir/link
print -r -- 'data' > $tgt
ln -s $tgt $lnk

local -a IN=( $lnk )
local -a A B
# Default is lstat: type should be l (symlink)
zppathstat A IN
case ${A[1]} in
  (*type=l*) ;;
  (*) print -u2 -- "expected symlink type: ${A[1]}"; exit 1;;
 esac
# With -L, follow to regular file: type should be f
zppathstat -L B IN
case ${B[1]} in
  (*type=f*) ;;
  (*) print -u2 -- "expected file type after -L: ${B[1]}"; exit 1;;
 esac

# Subcommand parity for -L
local -a C
zpmod pathstat -L C IN
case ${C[1]} in
  (*type=f*) ;;
  (*) print -u2 -- "subcommand -L mismatch: ${C[1]}"; exit 1;;
 esac

exit 0
