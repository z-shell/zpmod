#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

# Mixed endings: CRLF mid-file and trailing CR at end
# Content bytes: a\r\nb\rc\n  => a<CR><LF>b<CR>c<LF>
local f d
: ${TMPDIR:=/tmp}
d=${TMPDIR%/}/zpmod_rf_mix.$RANDOM
mkdir -p $d
f=$d/x
print -n -- $'a\r\nb\rc\n' > $f

# When splitting on CR, CRLF should be treated as a single separator,
# and lone CR should also separate.
local -a B
zpreadfile -d '\r' B $f
(( ${#B} == 3 ))
[[ ${B[1]} == 'a' ]]
[[ ${B[2]} == 'b' ]]
[[ ${B[3]} == $'c\n' ]]

# When splitting on LF, the CRs remain in the records before the LFs
local -a A
zpreadfile -d '\n' A $f
(( ${#A} == 2 ))
[[ ${A[1]} == $'a\r' ]]
[[ ${A[2]} == $'b\rc' ]]

exit 0
