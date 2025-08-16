#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

# Load staged zpmod
moddir=${ZPMOD_STAGE_MODULE_DIR:-}
if [[ -z $moddir ]]; then
  print -u2 "ZPMOD_STAGE_MODULE_DIR not set"
  exit 99
fi
module_path=($moddir $module_path)
zmodload -i zpmod

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

exit 0
