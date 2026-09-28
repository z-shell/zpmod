#!/usr/bin/env zsh
# zpreadarray rejects a -u argument that is not a whole non-negative integer
# instead of silently reading from fd 0 (atoi turned "abc" into 0)
set -euo pipefail
emulate -L zsh

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="builtin/zpreadarray_fd_invalid"
load_zpmod

typeset bad err
for bad in abc 3x '' -1 ' 3' 99999999999999999999; do
	err=$(zpreadarray -u "$bad" H 2>&1 </dev/null) && {
		print -r -- "accepted -u '$bad'" >&2
		exit 1
	}
	[[ $err == *"invalid file descriptor"* ]] || {
		print -r -- "unexpected error for -u '$bad': $err" >&2
		exit 1
	}
done

print -r -- "zpreadarray_fd_invalid OK"

test_status "PASS" "$TEST_NAME"
