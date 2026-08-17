#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="file_io/zpreadfile"
load_zpmod

local tdir=${TMPDIR:-$(mktemp -d)}
local f=$tdir/zpmod_rf_$$.txt
print -r -- 'a' > $f
print -r -- 'b' >> $f

local s
zpreadfile s $f
[[ $s == $'a\nb\n' ]]

local -a A
zpreadfile -d $'\n' A $f
(( ${#A} == 2 ))
[[ $A[1] == 'a' && $A[2] == 'b' ]]

test_status "PASS" "$TEST_NAME"
exit 0
