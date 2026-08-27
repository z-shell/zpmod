#!/usr/bin/env zsh
# Security regression coverage for file creation and cache replacement.
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/zpmod_secure_files"
load_zpmod

workdir=$(mktemp -d 2>/dev/null || mktemp -d -t zpmod-secure-files)
trap 'rm -rf -- "$workdir"' EXIT

fail_test() {
  print -ru2 -- "$*"
  exit 1
}

file_mode() {
  command stat -c %a -- "$1" 2>/dev/null || command stat -f %Lp "$1"
}

assert_mode() {
  local filepath=$1 expected=$2 actual
  actual=$(file_mode "$filepath")
  [[ $actual == $expected ]] ||
    fail_test "unexpected mode for $filepath: got $actual, expected $expected"
}

assert_victim_unchanged() {
  local victim_path=$1 expected=$2 operation=$3
  if "$operation" >/dev/null 2>&1; then
    fail_test "$operation followed a symlinked output"
  fi
  [[ $(<"$victim_path") == "$expected" ]] ||
    fail_test "$operation modified the symlink target"
}

# Deliberately remove the process umask as a safety net. Creation modes must
# still deny group/other writes.
umask 000

# fpath-index: public index data may be readable, but never group/other writable.
fdir="$workdir/fpath"
index="$workdir/fpath.index"
mkdir -p "$fdir"
print 'echo secure' >"$fdir/example"
saved_fpath=( $fpath )
fpath=( "$fdir" )
zpmod fpath-index --out "$index" --rebuild || fail_test "fpath-index failed"
assert_mode "$index" 644
fpath=( $saved_fpath )

# fpath-index must not follow a caller-supplied output symlink.
fdir="$workdir/fpath-symlink"
victim="$workdir/fpath-victim"
output="$workdir/fpath-link"
mkdir -p "$fdir"
print 'echo secure' >"$fdir/example"
print -r -- 'preserve-fpath-victim' >"$victim"
ln -s "$victim" "$output"
saved_fpath=( $fpath )
fpath=( "$fdir" )
_run_fpath_symlink() { zpmod fpath-index --out "$output" --rebuild }
assert_victim_unchanged "$victim" 'preserve-fpath-victim' _run_fpath_symlink
fpath=( $saved_fpath )

# rehash-diff snapshots contain PATH data and are private regardless of umask.
export XDG_CACHE_HOME="$workdir/rehash-cache"
mkdir -p "$XDG_CACHE_HOME"
zpmod rehash-diff >/dev/null || fail_test "rehash-diff failed"
assert_mode "$XDG_CACHE_HOME/zpmod/rehash_path_v1.snapshot" 600

# rehash-diff must not follow a cache-file symlink.
export XDG_CACHE_HOME="$workdir/rehash-symlink-cache"
mkdir -p "$XDG_CACHE_HOME/zpmod"
victim="$workdir/rehash-victim"
snapshot="$XDG_CACHE_HOME/zpmod/rehash_path_v1.snapshot"
print -r -- 'preserve-rehash-victim' >"$victim"
ln -s "$victim" "$snapshot"
_run_rehash_symlink() { zpmod rehash-diff }
assert_victim_unchanged "$victim" 'preserve-rehash-victim' _run_rehash_symlink

# compaudit caches contain filesystem security metadata and are private.
export XDG_CACHE_HOME="$workdir/compaudit-cache"
mkdir -p "$XDG_CACHE_HOME"
cdir="$workdir/compaudit-fpath"
mkdir -p "$cdir"
saved_fpath=( $fpath )
fpath=( "$cdir" )
zpmod compaudit-cache --rebuild >/dev/null || fail_test "compaudit rebuild failed"
assert_mode "$XDG_CACHE_HOME/zpmod/compaudit_v3.zcache" 600
fpath=( $saved_fpath )

# compaudit-cache must not follow a cache-file symlink.
export XDG_CACHE_HOME="$workdir/compaudit-symlink-cache"
mkdir -p "$XDG_CACHE_HOME/zpmod"
victim="$workdir/compaudit-victim"
cache="$XDG_CACHE_HOME/zpmod/compaudit_v3.zcache"
cdir="$workdir/compaudit-symlink-fpath"
mkdir -p "$cdir"
print -r -- 'preserve-compaudit-victim' >"$victim"
ln -s "$victim" "$cache"
saved_fpath=( $fpath )
fpath=( "$cdir" )
_run_compaudit_symlink() { zpmod compaudit-cache --rebuild }
assert_victim_unchanged "$victim" 'preserve-compaudit-victim' _run_compaudit_symlink
fpath=( $saved_fpath )

# Migration removes the legacy path atomically even when it is a dangling
# symlink. The symlink target must never be inspected or modified.
export XDG_CACHE_HOME="$workdir/compaudit-migration-cache"
mkdir -p "$XDG_CACHE_HOME/zpmod"
legacy="$XDG_CACHE_HOME/zpmod/compaudit_v2.zcache"
ln -s "$workdir/nonexistent-legacy-target" "$legacy"
cdir="$workdir/compaudit-migration-fpath"
mkdir -p "$cdir"
saved_fpath=( $fpath )
fpath=( "$cdir" )
zpmod compaudit-cache --show >/dev/null || fail_test "compaudit migration failed"
[[ ! -e "$legacy" && ! -L "$legacy" ]] ||
  fail_test "dangling legacy cache path was not removed"
fpath=( $saved_fpath )

test_status "PASS" "$TEST_NAME"
