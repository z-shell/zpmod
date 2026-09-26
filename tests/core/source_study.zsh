#!/usr/bin/env zsh
# Native Zsh test fixture for the sourced-script profiling interface.
emulate -R zsh

TEST_NAME="core/source_study"
source "${0:A:h}/../test_helpers.zsh" || exit 1

typeset scratch_dir rep rep_full rep_again invalid
typeset -a rows
scratch_dir=$(mktemp -d "${TMPDIR:-/tmp}/zpmod-srcstudy-XXXXXX") || exit 1
trap 'rm -rf -- "$scratch_dir"' EXIT
mkdir -p -- "$scratch_dir/home" "$scratch_dir/zdotdir" || exit 1
export HOME="$scratch_dir/home" ZDOTDIR="$scratch_dir/zdotdir"

load_zpmod || exit 1
assert_builtin_exists zpmod
rep=$(zpmod source-study) || exit 1
assert_equal "$rep" 'No source events recorded.'

print -r -- ':' > "$scratch_dir/a.zsh" || exit 1
print -r -- ':' > "$scratch_dir/b.zsh" || exit 1
. "$scratch_dir/a.zsh" || exit 1
source "$scratch_dir/b.zsh" || exit 1

rep=$(zpmod source-study) || exit 1
assert_contains "$rep" 'a.zsh'
assert_contains "$rep" 'b.zsh'
assert_contains "$rep" ' ms    '
assert_not_contains "$rep" "$scratch_dir"
rows=( "${(@f)rep}" )
assert_equal "${#rows}" 2

rep_full=$(zpmod source-study -l) || exit 1
assert_contains "$rep_full" "$scratch_dir/a.zsh"
assert_contains "$rep_full" "$scratch_dir/b.zsh"
rep_again=$(zpmod source-study) || exit 1
assert_equal "$rep_again" "$rep" '-l must not clear history'

# Explicit counts retain the newest entries, in completion order; zero is all.
rep=$(zpmod source-study 1) || exit 1
assert_contains "$rep" 'b.zsh'
assert_not_contains "$rep" 'a.zsh'
rep=$(zpmod source-study -l -- 1) || exit 1
assert_contains "$rep" "$scratch_dir/b.zsh"
rep=$(zpmod source-study 0) || exit 1
assert_equal "$rep" "$rep_again"

for invalid in -x -1 abc '' 9999999999999999999999999999; do
  if zpmod source-study "$invalid" >/dev/null 2>&1; then
    print -ru2 -- "unexpected success for argument: ${(qqq)invalid}"
    exit 1
  fi
done
if zpmod source-study 1 2 >/dev/null 2>&1 ||
   zpmod source-study -- 1 extra >/dev/null 2>&1; then
  print -ru2 -- 'unexpected success for excess operands'
  exit 1
fi

# Default output is not silently capped at the old stub parser's ten entries.
repeat 11; do
  source "$scratch_dir/a.zsh" || exit 1
done
rep=$(zpmod source-study) || exit 1
rows=( "${(@f)rep}" )
assert_equal "${#rows}" 13

print -r -- 'source_study OK'
