#!/usr/bin/env zsh
# Regression test for #31: sourcing a non-regular file (e.g. a process
# substitution FIFO/fd path) must not attempt ZWC compilation.
set -euo pipefail
emulate -L zsh

# Source helpers and set test name
source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="builtin/process_substitution_source"

# Load module
load_zpmod

workdir=$(mktemp -d)
trap 'rm -rf -- "$workdir"' EXIT

# Source a process substitution's /proc/self/fd/N path directly: the
# original #31 report. zpmod must not attempt to zcompile it (it can't be
# written back to under /proc), and it must execute silently either way.
unset nonregular_source_result
source <(print -r -- 'typeset -g nonregular_source_result=executed') 2>"$workdir/stderr.log"
nonregular_stderr=$(<"$workdir/stderr.log")

assert_equal "${nonregular_source_result:-missing}" "executed" \
  "Sourcing a process substitution should still execute its content"
assert_empty "$nonregular_stderr" \
  "Sourcing a process substitution must not warn about ZWC compilation"

# Same guard via a symlinked non-regular source (fifo), so a stray .zwc
# sibling landing in a normal, writable directory is easy to assert on too.
() {
  [[ ! -f "$1" ]] || return 1
  ln -s "$1" "$workdir/nonregular-source"
  source "$workdir/nonregular-source"
} <(print -r -- 'typeset -g nonregular_source_result=executed')

[[ ! -e "$workdir/nonregular-source.zwc" ]] ||
  { print -ru2 -- "unexpected: nonregular-source.zwc was compiled"; exit 1 }

print -r -- "process_substitution_source OK"

# Report
test_status "PASS" "$TEST_NAME"
test_info "process_substitution_source test completed successfully"
