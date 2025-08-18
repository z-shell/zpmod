#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="filesystem/zppathstat_fields"
load_zpmod

# Create a temp file to stat
local tdir=$(mktemp -d)
local f=$tdir/f
print -r -- 'x' > $f

local -a IN=( $f )
local -a OUT
zppathstat -f gid,ino OUT IN
(( ${#OUT} == 1 ))
# Only expect path=, gid=, ino=, and no other standard fields
[[ ${OUT[1]} == *gid=* ]] || { print -u2 -- "missing gid: ${OUT[1]}"; exit 1 }
[[ ${OUT[1]} == *ino=* ]] || { print -u2 -- "missing ino: ${OUT[1]}"; exit 1 }
case ${OUT[1]} in
	(*type=*|*size=*|*mode=*|*mtime=*) print -u2 -- "unexpected extra fields: ${OUT[1]}"; exit 1;;
	(*) ;;
esac

# Subcommand parity
local -a OUT2
zpmod pathstat -f gid,ino OUT2 IN
(( ${#OUT2} == 1 ))
[[ ${OUT2[1]} == ${OUT[1]} ]]

test_status "PASS" "$TEST_NAME"
exit 0
