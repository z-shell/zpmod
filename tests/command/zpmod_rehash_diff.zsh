#!/usr/bin/env zsh
# SPDX-License-Identifier: MIT
emulate -R zsh
setopt pipe_fail

typeset test_file=${0:A}
zpmod_test_main() {
  builtin emulate -L zsh
  setopt pipe_fail
  source "${test_file:h:h}/test_helpers.zsh" || exit 1
  TEST_NAME="command/zpmod_rehash_diff"

  fail_test() {
    builtin emulate -L zsh
    print -ru2 -- "$TEST_NAME: $*"
    exit 1
  }

  # Resolve fixture tools before replacing PATH with only test directories.
  typeset mkdir_tool touch_tool rm_tool
  mkdir_tool=$(whence -p mkdir) || fail_test "resolve mkdir"
  touch_tool=$(whence -p touch) || fail_test "resolve touch"
  rm_tool=$(whence -p rm) || fail_test "resolve rm"
  typeset test_tmp_parent=${TMPDIR:-/tmp}
  test_tmp_parent=${test_tmp_parent:A}
  typeset test_root
  test_root=$(mktemp -d "$test_tmp_parent/zpmod-rehash.XXXXXX") || exit 1
  TRAPEXIT() {
    if (( ZSH_SUBSHELL == 0 )) && [[ -d $test_root && $test_root == "$test_tmp_parent"/zpmod-rehash.* ]]; then
      "$rm_tool" -rf -- "$test_root"
    fi
  }
  "$mkdir_tool" -p "$test_root/home" "$test_root/cache" "$test_root/a" "$test_root/b" || fail_test "create isolated environment"
  export HOME="$test_root/home" ZDOTDIR="$test_root/home" XDG_CACHE_HOME="$test_root/cache"
  cd "$test_root" || fail_test "enter isolated directory"
  load_zpmod

  typeset dir_a="$test_root/a" dir_b="$test_root/b"
  # The implementation records whole-second directory mtimes. Fixed distinct
  # timestamps avoid inode reuse and same-second recreation hiding the change.
  "$touch_tool" -t 200001010000 "$dir_a" "$dir_b" || fail_test "set baseline metadata"
  path=( "$dir_b" )

  assert_report() {
    builtin emulate -L zsh
    typeset -a report_lines=( "${(@f)1}" )
    assert_equal "${report_lines[1]#*rehash-diff: }" "$2" "exact diff counts"
    if (( $# == 3 )); then
      assert_equal "${#report_lines}" 2 "one exact directory list"
      assert_equal "${report_lines[2]}" "$3"
    else
      assert_equal "${#report_lines}" 1 "unchanged PATH has no directory lists"
    fi
  }

  typeset output snapshot_header snapshot_file="$XDG_CACHE_HOME/zpmod/rehash_path_v1.snapshot"
  output=$(zpmod rehash-diff) || fail_test "baseline"
  assert_report "$output" 'added=1 removed=0 changed=0 unchanged=0' "  + dirs: $dir_b"
  assert_file_exists "$snapshot_file"
  IFS= read -r snapshot_header < "$snapshot_file" || fail_test "read snapshot header"
  assert_equal "$snapshot_header" 'version:1'

  output=$(zpmod rehash-diff) || fail_test "unchanged baseline"
  assert_report "$output" 'added=0 removed=0 changed=0 unchanged=1'

  path=( "$dir_a" "$dir_b" )
  output=$(zpmod rehash-diff) || fail_test "add directory"
  assert_report "$output" 'added=1 removed=0 changed=0 unchanged=1' "  + dirs: $dir_a"

  "$touch_tool" -t 200101010000 "$dir_a" || fail_test "change directory metadata"
  output=$(zpmod rehash-diff) || fail_test "metadata change"
  assert_report "$output" 'added=0 removed=0 changed=1 unchanged=1' "  * dirs: $dir_a"

  path=( "$dir_b" )
  output=$(zpmod rehash-diff) || fail_test "remove directory"
  assert_report "$output" 'added=0 removed=1 changed=0 unchanged=1' "  - dirs: $dir_a"

  output=$(zpmod rehash-diff) || fail_test "unchanged after removal"
  assert_report "$output" 'added=0 removed=0 changed=0 unchanged=1'
  test_status PASS "$TEST_NAME"
  exit 0
}
zpmod_test_main
