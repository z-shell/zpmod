#!/usr/bin/env zsh
# SPDX-License-Identifier: MIT

emulate -R zsh
setopt pipe_fail

fail() {
  print -ru2 -- "benchmark harness test: $*"
  exit 1
}

typeset repository_root=${0:A:h:h:h}
typeset module_dir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $module_dir && -d $module_dir ]] || fail "ZPMOD_STAGE_MODULE_DIR is not available"

typeset test_tmp_parent=${TMPDIR:-/tmp}
[[ -d $test_tmp_parent && -w $test_tmp_parent ]] || fail "temporary directory is not writable"
test_tmp_parent=${test_tmp_parent:A}
typeset test_root
test_root=$(mktemp -d "$test_tmp_parent/zpmod-benchmark-test.XXXXXX") || fail "could not create temporary directory"

cleanup_test() {
  if [[ -n ${test_root:-} && -d $test_root && $test_root == "$test_tmp_parent"/zpmod-benchmark-test.* ]]; then
    rm -rf -- "$test_root"
  fi
}
TRAPEXIT() {
  cleanup_test
}
TRAPINT() {
  cleanup_test
  exit 130
}
TRAPQUIT() {
  cleanup_test
  exit 131
}
TRAPTERM() {
  cleanup_test
  exit 143
}

# Docker overlay filesystems can report a false negative to zpmod's advisory
# access() precheck. Diagnostic mode bypasses that precheck; the harness still
# requires zcompile to create every expected .zwc file.
ZI_MOD_DEBUG=1 zsh -f "$repository_root/benchmarks/run.zsh" \
  --module-dir "$module_dir" \
  --output-dir "$test_root/output" \
  --source-revision test \
  --environment "benchmark smoke test" \
  --scripts 2 \
  --functions 2 \
  --warmups 1 \
  --runs 2 || fail "benchmark runner failed"

zsh -f "$repository_root/benchmarks/render.zsh" \
  --input "$test_root/output/benchmark.tsv" \
  --svg "$test_root/output/benchmark.svg" \
  --markdown "$test_root/output/benchmark.md" || fail "benchmark renderer failed"

typeset expected_file
for expected_file in benchmark.json benchmark.tsv benchmark.svg benchmark.md; do
  [[ -s "$test_root/output/$expected_file" ]] || fail "$expected_file was not generated"
done

grep -F '"schema_version": 1' "$test_root/output/benchmark.json" >/dev/null || fail "JSON schema version is missing"
grep -F $'result\tzpmod_warm\t' "$test_root/output/benchmark.tsv" >/dev/null || fail "warm result is missing"
grep -F '<title id="title">zpmod startup benchmark</title>' "$test_root/output/benchmark.svg" >/dev/null ||
  fail "SVG accessible title is missing"
grep -F '| zpmod warm |' "$test_root/output/benchmark.md" >/dev/null || fail "Markdown data table is missing"

print -r -- "benchmark harness OK"
