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
  TEST_NAME="command/zpmod_compaudit_cache"

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

  typeset secure_dir="$test_root/secure" insecure_dir="$test_root/insecure"
  mkdir -p "$secure_dir" "$insecure_dir" || fail_test "create fpath directories"
  chmod 0755 "$secure_dir" || fail_test "secure directory mode"
  chmod 0777 "$insecure_dir" || fail_test "insecure directory mode"
  # Remove inherited completion paths so counts and reasons describe this fixture.
  fpath=( "$secure_dir" "$insecure_dir" )
  typeset output cache_file="$XDG_CACHE_HOME/zpmod/compaudit_v3.zcache"
  output=$(zpmod compaudit-cache --rebuild --show) || fail_test "rebuild and show"
  assert_contains "$output" 'insecure 1 secure 1'
  assert_contains "$output" "! $insecure_dir"
  assert_file_exists "$cache_file"
  assert_equal "$(command head -n 1 "$cache_file")" 'version:3'

  command rm -- "$cache_file" || fail_test "remove isolated v3 fixture"
  print -r -- 'version:2' > "$XDG_CACHE_HOME/zpmod/compaudit_v2.zcache"
  zpmod compaudit-cache --show > "$test_root/migration.txt" || fail_test "migrate v2"
  assert_file_exists "$cache_file"
  assert_equal "$(command head -n 1 "$cache_file")" 'version:3'
  assert_file_not_exists "$XDG_CACHE_HOME/zpmod/compaudit_v2.zcache" "legacy cache removed"

  # Parse JSON and inspect the exact directory entry, rather than matching an
  # unrelated directory's reason elsewhere in the document.
  assert_json_entry() {
    builtin emulate -L zsh
    python3 - "$1" "$2" "$3" "$4" <<'PYJSON' || fail_test "JSON entry assertion"
import json, sys
with open(sys.argv[1], encoding="utf-8") as stream:
    data = json.load(stream)
assert isinstance(data["insecure"], int) and isinstance(data["secure"], int)
entries = [entry for entry in data["dirs"] if entry["path"] == sys.argv[2]]
assert len(entries) == 1, entries
entry = entries[0]
assert entry["verdict"] == int(sys.argv[3]), entry
reason = sys.argv[4]
assert (reason in entry["reasons"]) if reason else not entry["reasons"], entry
PYJSON
  }
  zpmod compaudit-cache --rebuild --json > "$test_root/baseline.json" || fail_test "JSON rebuild"
  assert_json_entry "$test_root/baseline.json" "$insecure_dir" 1 dir_perms
  assert_json_entry "$test_root/baseline.json" "$secure_dir" 0 ''

  chmod 0777 "$secure_dir" || fail_test "change directory permissions"
  output=$(zpmod compaudit-cache --show) || fail_test "incremental show"
  assert_contains "$output" 'insecure 2 secure 0'
  assert_contains "$output" "! $secure_dir"
  assert_contains "$output" "! $insecure_dir"

  mkdir -p "$test_root/parent/child" || fail_test "create ancestor fixture"
  chmod 0755 "$test_root/parent" "$test_root/parent/child" || fail_test "secure ancestor modes"
  fpath=( "$test_root/parent/child" )
  zpmod compaudit-cache --rebuild --json > "$test_root/ancestor-before.json" || fail_test "ancestor baseline"
  assert_json_entry "$test_root/ancestor-before.json" "$test_root/parent/child" 0 ''
  chmod 0777 "$test_root/parent" || fail_test "change ancestor permissions"
  zpmod compaudit-cache --json > "$test_root/ancestor-after.json" || fail_test "ancestor incremental update"
  assert_json_entry "$test_root/ancestor-after.json" "$test_root/parent/child" 1 ancestor_perms

  mkdir -p "$test_root/zwcdir" || fail_test "create zwc fixture"
  chmod 0755 "$test_root/zwcdir" || fail_test "secure zwc directory"
  print -r -- '' > "$test_root/zwcdir/test.zwc"
  chmod 0666 "$test_root/zwcdir/test.zwc" || fail_test "insecure zwc mode"
  fpath=( "$test_root/zwcdir" )
  zpmod compaudit-cache --rebuild --json > "$test_root/zwc.json" || fail_test "zwc JSON rebuild"
  assert_json_entry "$test_root/zwc.json" "$test_root/zwcdir" 1 zwc_perms
  # Changing only a compiled file's permissions must invalidate the verdict.
  chmod 0644 "$test_root/zwcdir/test.zwc" || fail_test "secure zwc mode"
  zpmod compaudit-cache --json > "$test_root/zwc-fixed.json" || fail_test "zwc permission refresh"
  assert_json_entry "$test_root/zwc-fixed.json" "$test_root/zwcdir" 0 ''
  chmod 0666 "$test_root/zwcdir/test.zwc" || fail_test "insecure zwc mode again"
  zpmod compaudit-cache --json > "$test_root/zwc-changed.json" || fail_test "zwc permission invalidation"
  assert_json_entry "$test_root/zwc-changed.json" "$test_root/zwcdir" 1 zwc_perms
  test_status PASS "$TEST_NAME"

  exit 0
}
zpmod_test_main
