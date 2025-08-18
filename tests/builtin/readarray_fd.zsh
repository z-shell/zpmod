#!/usr/bin/env zsh
# readarray from fd with custom delimiter
set -euo pipefail
emulate -L zsh

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="builtin/readarray_fd"
load_zpmod

H=()
{
	exec {fd}<> <(print -nr -- 'aa;bb;cc;')
	readarray -d ';' -u $fd H
	exec {fd}>&-
}
(( ${#H[@]} == 4 ))
[[ $H[1] == 'aa;' && $H[2] == 'bb;' && $H[3] == 'cc;' && $H[4] == '' ]]

print -r -- "readarray_fd OK"

test_status "PASS" "$TEST_NAME"
