#!/usr/bin/env zsh
# Validate zpmod report-append uses zsh allocators and appends correctly
set -euo pipefail
emulate -L zsh

# Source helpers and load module
source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/zpmod_report_append"
load_zpmod

typeset -gA ZI_REPORTS
ZI_REPORTS=()

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
