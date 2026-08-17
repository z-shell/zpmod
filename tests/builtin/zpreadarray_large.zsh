#!/usr/bin/env zsh
# Stress test: many records to exercise buffer growth and callback cadence
set -euo pipefail
emulate -L zsh

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="builtin/zpreadarray_large"
load_zpmod

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
zpreadarray -C : -c 5000 -u {fd} ARR {fd}< $TMPFILE

(( ${#ARR[@]} == N ))
[[ $ARR[1] == r1 && $ARR[$N] == r$N ]]

print -r -- "zpreadarray_large OK"

test_status "PASS" "$TEST_NAME"
