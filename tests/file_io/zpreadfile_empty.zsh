#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="file_io/zpreadfile_empty"
load_zpmod

: ${TMPDIR:=/tmp}
d=${TMPDIR%/}/zpmod_rf_empty.$RANDOM
mkdir -p $d
f=$d/x
: > $f  # empty file

# Scalar: empty string
local S
zpreadfile S $f
[[ -n ${+S} ]]
[[ $S == '' ]]

# Array split: length 0
local -a A
zpreadfile -d $'\n' A $f
(( ${#A} == 0 ))

test_status "PASS" "$TEST_NAME"
exit 0
