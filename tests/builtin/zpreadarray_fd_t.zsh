#!/usr/bin/env zsh
# zpreadarray from fd with custom delimiter and trimming (-t)
set -euo pipefail
emulate -L zsh

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="builtin/zpreadarray_fd_t"
load_zpmod

ARR=()
{
	exec {fd}<> <(print -nr -- 'A;B;C;')
	zpreadarray -t -d ';' -u $fd ARR
	exec {fd}>&-
}

# Expect trimming of delimiter, but keep trailing empty record
(( ${#ARR[@]} == 4 ))
[[ $ARR[1] == 'A' && $ARR[2] == 'B' && $ARR[3] == 'C' && $ARR[4] == '' ]]

print -r -- "zpreadarray_fd_t OK"

test_status "PASS" "$TEST_NAME"
