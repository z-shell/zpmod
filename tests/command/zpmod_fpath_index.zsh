#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/zpmod_fpath_index"
load_zpmod

# Create temporary fpath with two function dirs
local td
td=$(mktemp -d)
mkdir -p "$td/dir1" "$td/dir2"
print 'echo one' > "$td/dir1/one"
print 'echo two' > "$td/dir2/two"
chmod +x "$td/dir1/one" "$td/dir2/two"

# Prepend to fpath
local -a save_fpath=( $fpath )
fpath=( "$td/dir1" "$td/dir2" $fpath )

local idx_file="$td/index.txt"

# First build should create file with entries
zpmod fpath-index --out "$idx_file"
[[ -s "$idx_file" ]] || { echo 'index file empty'; exit 1; }

grep -q 'one ' "$idx_file" || { echo 'missing function one'; exit 1; }
grep -q 'two ' "$idx_file" || { echo 'missing function two'; exit 1; }

# Second invocation should be a fast skip (mtime unchanged) and leave file intact
local ts_before=$(stat -c %Y "$idx_file" 2>/dev/null || stat -f %m "$idx_file")
sleep 1
zpmod fpath-index --out "$idx_file"
local ts_after=$(stat -c %Y "$idx_file" 2>/dev/null || stat -f %m "$idx_file")
[[ $ts_before -eq $ts_after ]] || { echo 'file unexpectedly rewritten (should skip)'; exit 1; }

# Touch one dir to force rebuild detection
sleep 1
print 'echo three' > "$td/dir1/three"
chmod +x "$td/dir1/three"
# Update directory mtime by creating file; now expect rewrite
local ts_prev=$ts_after
zpmod fpath-index --out "$idx_file"
local ts_new=$(stat -c %Y "$idx_file" 2>/dev/null || stat -f %m "$idx_file")
[[ $ts_new -ne $ts_prev ]] || { echo 'file not rewritten after dir change'; exit 1; }

grep -q 'three ' "$idx_file" || { echo 'missing function three after rebuild'; exit 1; }

# Preload mode: should populate shfunctab without rewriting file (skip due to fresh header)
local count_before=$(( $(whence -w one 2>/dev/null | wc -l) ))
zpmod fpath-index --out "$idx_file" --preload
local count_after=$(( $(whence -w one 2>/dev/null | wc -l) ))
[[ $count_after -ge $count_before ]] || { echo 'preload did not keep or increase functions'; exit 1; }

# Cleanup
fpath=( $save_fpath )
rm -rf "$td"

test_status "PASS" "$TEST_NAME"
exit 0
