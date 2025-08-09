#!/usr/bin/env zsh
# Validate zpmod report-append uses zsh allocators and appends correctly
set -euo pipefail
emulate -L zsh

: ${ZPMOD_STAGE_MODULE_DIR:?ZPMOD_STAGE_MODULE_DIR is required}
module_path=("$ZPMOD_STAGE_MODULE_DIR" $module_path)

zmodload -i zpmod

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
