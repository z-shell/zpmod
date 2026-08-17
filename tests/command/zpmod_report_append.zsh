#!/usr/bin/env zsh
# Validate zpmod report-append uses zsh allocators and appends correctly
set -euo pipefail
emulate -L zsh

# Source helpers and load module
source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/zpmod_report_append"
load_zpmod

typeset -gA ZI_REPORTS

# A declared associative parameter has no hash table until it is initialized.
# report-append must reject that state without dereferencing a null table.
if zpmod report-append z-shell/not-registered '+first'; then
  print -u2 -- 'report-append accepted an uninitialized ZI_REPORTS hash'
  exit 1
fi

ZI_REPORTS=()

# An explicit empty entry is valid and must be appendable.
ZI_REPORTS['z-shell/empty']=''
zpmod report-append z-shell/empty '+one'
[[ ${ZI_REPORTS['z-shell/empty']} == '+one' ]]

# Seed
ZI_REPORTS["z-shell/zbrowse"]='seed'

# Append new body
zpmod report-append z-shell/zbrowse '+one'
[[ ${ZI_REPORTS["z-shell/zbrowse"]} == 'seed+one' ]]

# Append again
zpmod report-append z-shell/zbrowse '+two'
[[ ${ZI_REPORTS["z-shell/zbrowse"]} == 'seed+one+two' ]]

print -r -- "zpmod_report_append OK"

# Report
test_status "PASS" "$TEST_NAME"
test_info "zpmod_report_append test completed successfully"
