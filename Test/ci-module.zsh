#!/usr/bin/env zsh

emulate -LR zsh
setopt err_exit no_unset pipe_fail

if (( $# != 1 )); then
  print -u2 "Usage: ${0:t} MODULE_DIR"
  exit 2
fi

typeset -r module_dir=${1:A}
if [[ ! -d $module_dir ]]; then
  print -u2 "Module directory does not exist: $module_dir"
  exit 1
fi

typeset temp_dir
temp_dir=$(mktemp -d "${TMPDIR:-/tmp}/zpmod-ci.XXXXXX")
trap 'rm -rf -- "$temp_dir"' EXIT

module_path=( "$module_dir" "${module_path[@]}" )
zmodload zi/zpmod

if (( ! ${+builtins[zpmod]} )); then
  print -u2 "The zpmod builtin was not registered"
  exit 1
fi

typeset usage
usage=$(zpmod -h)
if [[ $usage != *"zpmod source-study"* ]]; then
  print -u2 "The zpmod help output is incomplete"
  exit 1
fi

typeset -gA ZI_REPORTS
ZI_REPORTS[ci/module]=""
zpmod report-append ci/module first
zpmod report-append ci/module second
if [[ ${ZI_REPORTS[ci/module]} != firstsecond ]]; then
  print -u2 "The report-append command returned unexpected data"
  exit 1
fi

typeset -r fixture=$temp_dir/fixture.zsh
print -r -- 'typeset -g ZPMOD_CI_FIXTURE=loaded' > "$fixture"
source "$fixture"

if [[ ${ZPMOD_CI_FIXTURE:-} != loaded ]]; then
  print -u2 "The module did not source the fixture"
  exit 1
fi

if [[ ! -s $fixture.zwc ]]; then
  print -u2 "The module did not compile the sourced fixture"
  exit 1
fi

typeset source_report
source_report=$(zpmod source-study -l)
if [[ $source_report != *"$fixture"* ]]; then
  print -u2 "The source-study report did not include the fixture"
  exit 1
fi
