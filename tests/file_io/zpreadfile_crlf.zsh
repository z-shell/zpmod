#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="file_io/zpreadfile_crlf"
load_zpmod

local f d
: ${TMPDIR:=/tmp}
d=${TMPDIR%/}/zpmod_rf_crlf.$RANDOM
mkdir -p $d
f=$d/x
# Write CRLF lines (real CR and LF bytes)
print -n -- $'a\r\nb\r\n' > $f

local -a A
zpreadfile -d $'\n' A $f   # split by LF
(( ${#A} == 2 ))
[[ ${A[1]} == $'a\r' ]]
[[ ${A[2]} == $'b\r' ]]

local -a B
zpreadfile -d $'\r' B $f   # split by CR
(( ${#B} == 2 ))
[[ ${B[1]} == 'a' ]]
[[ ${B[2]} == 'b' ]]

test_status "PASS" "$TEST_NAME"
exit 0
