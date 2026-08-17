#!/usr/bin/env zsh
# Test incremental PATH diff snapshot logic (initial slice).
0=${(%):-%N}
this_dir=${0:A:h}
source "$this_dir/../test_helpers.zsh" || exit 1

setup_module || fail "module load failed"

# Work in isolated PATH modifications
local -a save_path
save_path=(${path[@]})

wd=$(mktemp -d 2>/dev/null || mktemp -d -t tmp)
newdir_a=$wd/a
newdir_b=$wd/b
mkdir -p $newdir_a $newdir_b || fail "mkdir failed"

# First run with original PATH (snapshot baseline)
zpmod rehash-diff >$wd/out1.txt || fail "rehash-diff baseline failed"

# Add a directory and run again
path=($newdir_a $path)
zpmod rehash-diff >$wd/out2.txt || fail "rehash-diff add failed"
grep -q "+ dirs:" $wd/out2.txt || fail "expected + dirs list"

# Force metadata change: touch inode/mtime by recreating directory
rmdir $newdir_a || fail "rmdir failed"
mkdir -p $newdir_a || fail "recreate failed"
zpmod rehash-diff >$wd/out3.txt || fail "rehash-diff change failed"
grep -q "changed=" $wd/out3.txt || fail "expected changed summary"

# Remove dir from PATH
path=(${path:#$newdir_a})
zpmod rehash-diff >$wd/out4.txt || fail "rehash-diff remove failed"
grep -q "removed=" $wd/out4.txt || fail "expected removed summary"

# Clean up
path=(${save_path[@]})
rm -rf -- $wd

success "rehash-diff tests ok"
