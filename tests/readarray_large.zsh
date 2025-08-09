#!/usr/bin/env zsh
# Stress test: many records to exercise buffer growth and callback cadence
set -euo pipefail
emulate -L zsh

: ${ZPMOD_STAGE_MODULE_DIR:?ZPMOD_STAGE_MODULE_DIR is required}
module_path=("$ZPMOD_STAGE_MODULE_DIR" $module_path)

zmodload -i zpmod

N=20000
TMPFILE=$(mktemp)
trap 'rm -f -- $TMPFILE' EXIT

{
  i=1
  while (( i <= N )); do
    print -r -- "r$i" >> $TMPFILE
    (( i++ ))
  done
}

ARR=()
# Callback every 5000 to ensure no pathological slowdown
readarray -C : -c 5000 -u {fd} ARR {fd}< $TMPFILE

(( ${#ARR[@]} == N ))
[[ $ARR[1] == r1 && $ARR[$N] == r$N ]]

print -r -- "readarray_large OK"
