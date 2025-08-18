#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/zpmod_subcommands"
load_zpmod

# dirlist parity
local tdir
tdir=$(mktemp -d)
mkdir -p $tdir/dirA
print -r -- fileA > $tdir/fileA
local -a A B
zpdirlist A $tdir
zpmod dirlist B $tdir
[[ ${#A} -eq ${#B} ]]

# pathstat parity (just check both produce some output)
local -a IN=( $tdir $tdir/fileA )
local -a S1 S2
zppathstat S1 IN
zpmod pathstat S2 IN
(( ${#S1} == ${#S2} ))
[[ $S1[1] == (path=*) ]]

# readfile parity
local f=$tdir/x
print -r -- 'a' > $f
print -r -- 'b' >> $f
local s1 s2
zpreadfile s1 $f
zpmod readfile s2 $f
[[ $s1 == $s2 ]]

test_status "PASS" "$TEST_NAME"
exit 0
