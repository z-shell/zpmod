#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

# Use helpers to load staged zpmod
source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="filesystem/zpdirlist"
load_zpmod

tmpdir=${TMPDIR:-$(mktemp -d)}
testdir=$tmpdir/zpmod_t_dir
mkdir -p $testdir
print -r -- file1 > $testdir/file1
mkdir -p $testdir/sub

local -a out
zpdirlist out $testdir
(( ${#out} >= 1 ))
# Should not include dotfiles by default
for e in $out; do
	[[ ${e[1]} != '.' ]]
done

# Only files
local -a files
zpdirlist -f files $testdir
[[ $files == file1 ]]

# Only dirs
local -a dirs
zpdirlist -d dirs $testdir
[[ $dirs == sub ]]

test_status "PASS" "$TEST_NAME"
exit 0
