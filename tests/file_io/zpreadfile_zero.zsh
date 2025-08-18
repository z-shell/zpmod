#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="file_io/zpreadfile_zero"
load_zpmod

# Ensure no user alias/function shadows the builtin name
unalias zpreadfile 2>/dev/null || true
unset -f zpreadfile 2>/dev/null || true

# On error, dump diagnostics to help investigate intermittent failures
TRAPZERR() {
	local rc=$?
	{
		print -u2 -- "[TRAPZERR] rc=$rc line=$LINENO funcstack=($funcstack)"
		print -u2 -- "[TRAPZERR] zsh=$ZSH_VERSION options: ${(j: :)${(ok)options}}"
		print -u2 -- "[TRAPZERR] whence -w zpreadfile:"
		whence -w zpreadfile 2>&2 || true
		print -u2 -- "[TRAPZERR] type -w zpreadfile:"
		type -w zpreadfile 2>&2 || true
		print -u2 -- "[TRAPZERR] alias zpreadfile:"
		alias zpreadfile 2>&2 || true
		print -u2 -- "[TRAPZERR] functions -M zpreadfile:"
		functions -M zpreadfile 2>&2 || true
	} 2>/dev/null || true
	return $rc
}

local tdir
tdir=$(mktemp -d)
local f=$tdir/zpmod_rf0_$$.bin

# Create NUL-delimited content: "a\0b\0c"
: >| "$f"
print -rn -- a >| "$f"
print -rn -- $'\0' >> "$f"
print -rn -- b >> "$f"
print -rn -- $'\0' >> "$f"
print -rn -- c >> "$f"

local -a A
# Use explicit NUL delimiter via -d '\0' to avoid any edge cases with -0 parsing
zpmod readfile -d '\0' A "$f"
(( ${#A} == 3 ))
[[ $A[1] == a && $A[2] == b && $A[3] == c ]]

test_status "PASS" "$TEST_NAME"
exit 0
