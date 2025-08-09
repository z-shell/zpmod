#!/usr/bin/env zsh
# readarray from fd with custom delimiter
set -euo pipefail
emulate -L zsh

: ${ZPMOD_STAGE_MODULE_DIR:?ZPMOD_STAGE_MODULE_DIR is required}
module_path=("$ZPMOD_STAGE_MODULE_DIR" $module_path)

zmodload -i zpmod

H=()
{
  exec {fd}<> <(print -nr -- 'aa;bb;cc;')
  readarray -d ';' -u $fd H
  exec {fd}>&-
}
(( ${#H[@]} == 4 ))
[[ $H[1] == 'aa;' && $H[2] == 'bb;' && $H[3] == 'cc;' && $H[4] == '' ]]

print -r -- "readarray_fd OK"
