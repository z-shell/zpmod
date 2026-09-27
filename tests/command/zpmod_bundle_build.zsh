#!/usr/bin/env zsh
# SPDX-License-Identifier: MIT
emulate -R zsh
setopt pipe_fail

# Keep fixture redirections inside a function so native syntax-only checks
# cannot expand unset test paths or create files.
typeset test_file=${0:A}
zpmod_test_main() {
  builtin emulate -L zsh
  setopt pipe_fail
  source "${test_file:h:h}/test_helpers.zsh" || exit 1
  TEST_NAME="command/zpmod_bundle_build"

  fail_test() {
    builtin emulate -L zsh
    print -ru2 -- "$TEST_NAME: $*"
    exit 1
  }

  typeset test_tmp_parent=${TMPDIR:-/tmp}
  test_tmp_parent=${test_tmp_parent:A}
  typeset test_root
  test_root=$(mktemp -d "$test_tmp_parent/zpmod-command.XXXXXX") || exit 1
  TRAPEXIT() {
    if (( ZSH_SUBSHELL == 0 )) && [[ -d $test_root && $test_root == "$test_tmp_parent"/zpmod-command.* ]]; then
      command rm -rf -- "$test_root"
    fi
  }
  # Keep cache writes and startup state outside the invoking user's environment.
  mkdir -p "$test_root/home" "$test_root/cache" || fail_test "create isolated environment"
  export HOME="$test_root/home" ZDOTDIR="$test_root/home" XDG_CACHE_HOME="$test_root/cache"
  cd "$test_root" || fail_test "enter isolated directory"
  load_zpmod

  typeset source_dir="$test_root/sources" bundle="$test_root/bundle.zsh" output
  mkdir -p "$source_dir/beta" "$source_dir/alpha" || fail_test "create source tree"
  # Creation order differs from lexical order, and two nested files exceed 1 KiB.
  print -r -- "# ${(l:800::a:)}" > "$source_dir/alpha/01-first.zsh"
  print -r -- "# ${(l:800::b:)}" > "$source_dir/alpha/02-second.plugin.zsh"
  print -r -- 'print -r -- beta' > "$source_dir/beta/last.zsh"
  print -r -- 'ignored text' > "$source_dir/ignored.txt"
  zpmod bundle-build --from "$source_dir" --out "$bundle" || fail_test "recursive build"
  assert_file_exists "$bundle"
  output=$(<"$bundle")
  assert_contains "$output" 'BEGIN alpha/01-first.zsh'
  assert_contains "$output" 'BEGIN alpha/02-second.plugin.zsh'
  assert_contains "$output" 'BEGIN beta/last.zsh'
  assert_not_contains "$output" 'ignored text'
  typeset -a markers
  markers=( "${(@f)$(command grep 'BEGIN ' "$bundle")}" )
  assert_equal "${#markers}" 3 "all recursive sources emitted"
  assert_contains "$markers[1]" 'alpha/01-first.zsh'
  assert_contains "$markers[2]" 'alpha/02-second.plugin.zsh'
  assert_contains "$markers[3]" 'beta/last.zsh'

  zpmod bundle-build --from "$source_dir" --out "$test_root/capped.zsh" --max 1 2> "$test_root/cap.err" || fail_test "capped build"
  output=$(<"$test_root/capped.zsh")
  assert_contains "$output" 'BEGIN alpha/01-first.zsh'
  assert_not_contains "$output" 'BEGIN alpha/02-second.plugin.zsh'
  assert_not_contains "$output" 'BEGIN beta/last.zsh'
  assert_contains "$(<"$test_root/cap.err")" 'truncated'

  file_mtime() {
    builtin emulate -L zsh
    command stat -c %Y -- "$1" 2>/dev/null || command stat -f %m "$1"
  }
  typeset before after
  before=$(file_mtime "$bundle") || fail_test "read bundle timestamp"
  sleep 1
  zpmod bundle-build --from "$source_dir" --out "$bundle" || fail_test "freshness check"
  after=$(file_mtime "$bundle") || fail_test "read unchanged timestamp"
  assert_equal "$after" "$before" "fresh bundle must not be rewritten"
  sleep 1
  print -r -- '# changed source' >> "$source_dir/alpha/01-first.zsh"
  zpmod bundle-build --from "$source_dir" --out "$bundle" || fail_test "rebuild changed source"
  after=$(file_mtime "$bundle") || fail_test "read rebuilt timestamp"
  assert_greater_than "$after" "$before" "changed source must rebuild bundle"
  assert_contains "$(<"$bundle")" '# changed source'
  # Output inside the input tree must never become an input on rebuild.
  zpmod bundle-build --from "$source_dir" --out "$source_dir/inside.zsh" || fail_test "output inside source tree"
  sleep 1
  print -r -- '# another change' >> "$source_dir/alpha/01-first.zsh"
  zpmod bundle-build --from "$source_dir" --out "$source_dir/inside.zsh" || fail_test "rebuild output inside source tree"
  assert_not_contains "$(<"$source_dir/inside.zsh")" 'BEGIN inside.zsh'

  # Collection and output validation failures preserve existing output and targets.
  print -r -- 'preserve target' > "$test_root/victim"
  ln -s "$test_root/victim" "$test_root/link" || fail_test "create output symlink"
  if zpmod bundle-build --from "$source_dir" --out "$test_root/link" 2>/dev/null; then
    fail_test "output symlink accepted"
  fi
  assert_equal "$(<"$test_root/victim")" 'preserve target'
  if zpmod bundle-build --from "$test_root/missing" --out "$test_root/victim" 2>/dev/null; then
    fail_test "missing source accepted"
  fi
  assert_equal "$(<"$test_root/victim")" 'preserve target'
  # A directory symlink cycle is ignored rather than recursively followed.
  ln -s "$source_dir" "$source_dir/beta/cycle" || fail_test "create cycle fixture"
  zpmod bundle-build --from "$source_dir" --out "$test_root/no-cycle.zsh" || fail_test "symlink cycle handling"

  typeset invalid_limit
  for invalid_limit in '' -1 invalid 999999999999999999999999999; do
    if zpmod bundle-build --from "$source_dir" --out "$test_root/victim" --max "$invalid_limit" 2>/dev/null; then
      fail_test "invalid size limit accepted"
    fi
    assert_equal "$(<"$test_root/victim")" 'preserve target'
  done
  test_status PASS "$TEST_NAME"

  exit 0
}
zpmod_test_main
