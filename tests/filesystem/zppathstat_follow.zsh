#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="filesystem/zppathstat_follow"
load_zpmod

local tdir=$(mktemp -d)
local tgt=$tdir/real
local lnk=$tdir/link
print -r -- 'data' > $tgt
ln -s $tgt $lnk

local -a IN=( $lnk )
local -a A B
# Default is lstat: type should be l (symlink)
zppathstat A IN
case ${A[1]} in
	(*type=l*) ;;
	(*) print -u2 -- "expected symlink type: ${A[1]}"; exit 1;;
 esac
# With -L, follow to regular file: type should be f
zppathstat -L B IN
case ${B[1]} in
	(*type=f*) ;;
	(*) print -u2 -- "expected file type after -L: ${B[1]}"; exit 1;;
 esac

# Subcommand parity for -L
local -a C
zpmod path-stat -L C IN
case ${C[1]} in
	(*type=f*) ;;
	(*) print -u2 -- "subcommand -L mismatch: ${C[1]}"; exit 1;;
 esac

test_status "PASS" "$TEST_NAME"
exit 0
