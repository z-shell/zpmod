#!/usr/bin/env zsh
# zpreadarray from fd with custom delimiter
set -euo pipefail
emulate -L zsh

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="builtin/zpreadarray_fd"
load_zpmod

H=()
{
	exec {fd}<> <(print -nr -- 'aa;bb;cc;')
	zpreadarray -d ';' -u $fd H
	exec {fd}>&-
}
(( ${#H[@]} == 4 ))
[[ $H[1] == 'aa;' && $H[2] == 'bb;' && $H[3] == 'cc;' && $H[4] == '' ]]

print -r -- "zpreadarray_fd OK"

test_status "PASS" "$TEST_NAME"
