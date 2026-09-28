#!/usr/bin/env zsh
# zpmod source-hot rejects -n and --threshold values that are not whole
# integers instead of treating them as 0 (atoi turned "abc" into 0)
set -euo pipefail
emulate -L zsh

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/source_hot_args"
load_zpmod

typeset opt bad err
for opt in -n --threshold; do
	for bad in abc 10x '' 99999999999999999999; do
		err=$(zpmod source-hot $opt "$bad" 2>&1) && {
			print -r -- "accepted $opt '$bad'" >&2
			exit 1
		}
		[[ $err == *"$opt: invalid number"* ]] || {
			print -r -- "unexpected error for $opt '$bad': $err" >&2
			exit 1
		}
	done
done

# Well-formed values are still accepted.
zpmod source-hot -n 5 --threshold 0 >/dev/null

print -r -- "source_hot_args OK"

test_status "PASS" "$TEST_NAME"
