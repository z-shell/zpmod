#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

local -a IN=( /definitely/not/here )
local -a OUT
# Request only mode,mtime; on error only errno (+ optionally type if requested) should appear
zppathstat -f mode,mtime OUT IN
(( ${#OUT} == 1 ))
case ${OUT[1]} in
  (path=*,errno=*) ;;
  (*) print -u2 -- "unexpected fields: ${OUT[1]}"; exit 1 ;;
esac

# Request type explicitly: type should appear along with errno
zppathstat -f type OUT IN
case ${OUT[1]} in
  (path=*,type=?,errno=*) ;;
  (*) print -u2 -- "missing type or errno: ${OUT[1]}"; exit 1 ;;
esac

exit 0
