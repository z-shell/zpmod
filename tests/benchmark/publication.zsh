#!/usr/bin/env zsh
# SPDX-License-Identifier: MIT

emulate -R zsh
setopt pipe_fail

fail() {
  print -ru2 -- "benchmark publication test: $*"
  exit 1
}

typeset repository_root=${0:A:h:h:h}
typeset result_dir="$repository_root/benchmarks/results/v2.0.6-linux-x86_64"
typeset result_tsv="$result_dir/benchmark.tsv"
[[ -f $result_tsv ]] || fail "published TSV result is missing"

typeset test_tmp_parent=${TMPDIR:-/tmp}
[[ -d $test_tmp_parent && -w $test_tmp_parent ]] || fail "temporary directory is not writable"
test_tmp_parent=${test_tmp_parent:A}
typeset test_root
test_root=$(mktemp -d "$test_tmp_parent/zpmod-benchmark-publication.XXXXXX") || fail "could not create temporary directory"

cleanup_test() {
  if [[ -n ${test_root:-} && -d $test_root && $test_root == "$test_tmp_parent"/zpmod-benchmark-publication.* ]]; then
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

zsh -f "$repository_root/benchmarks/render.zsh" \
  --input "$result_tsv" \
  --svg "$test_root/benchmark.svg" \
  --markdown "$test_root/benchmark.md" || fail "could not regenerate published views"

cmp -s "$test_root/benchmark.svg" "$result_dir/benchmark.svg" || fail "published SVG has drifted from the TSV source"
cmp -s "$test_root/benchmark.md" "$result_dir/benchmark.md" || fail "published Markdown has drifted from the TSV source"

typeset record_type case_id label median p95 remaining
while IFS=$'\t' read -r record_type case_id label median p95 remaining; do
  [[ $record_type == result ]] || continue
  grep -F "$median ms" "$repository_root/README.md" >/dev/null || fail "README is missing median $median for $case_id"
  grep -F "$p95 ms" "$repository_root/README.md" >/dev/null || fail "README is missing p95 $p95 for $case_id"
done < "$result_tsv"

print -r -- "benchmark publication OK"
