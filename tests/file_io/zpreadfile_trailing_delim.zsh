#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="file_io/zpreadfile_trailing_delim"
load_zpmod

# LF case: a\nb\n -> two elements a, b
local f d
: ${TMPDIR:=/tmp}
d=${TMPDIR%/}/zpmod_rf_trail.$RANDOM
mkdir -p $d
f=$d/x
print -n -- $'a\nb\n' > $f
local -a A
zpreadfile -d $'\n' A $f
(( ${#A} == 2 ))
[[ ${A[1]} == 'a' ]]
[[ ${A[2]} == 'b' ]]

# CR case: a\rb\r -> two elements a, b
print -n -- $'a\rb\r' > $f
local -a B
zpreadfile -d $'\r' B $f
(( ${#B} == 2 ))
[[ ${B[1]} == 'a' ]]
[[ ${B[2]} == 'b' ]]

test_status "PASS" "$TEST_NAME"

exit 0
