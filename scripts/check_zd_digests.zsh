#!/usr/bin/env zsh
# SPDX-License-Identifier: MIT
# Fails when the qualified zd image digests in .github/workflows/zd-controlled.yml
# and benchmarks/README.md differ. The workflow is the source of truth.
# Usage: check_zd_digests.zsh [WORKFLOW README]

set -eu
root_dir=${0:A:h}/..
workflow=${1:-$root_dir/.github/workflows/zd-controlled.yml}
readme=${2:-$root_dir/benchmarks/README.md}

typeset -a from_workflow from_readme
from_workflow=("${(@f)$(sed -n -E 's/.*"zsh": "([0-9.]+)", "image": "([^"]+@sha256:[0-9a-f]{64})".*/\1 \2/p' "$workflow")}")
from_readme=("${(@f)$(sed -n -E 's/^([0-9.]+): ([^ ]+@sha256:[0-9a-f]{64})$/\1 \2/p' "$readme")}")
from_workflow=(${from_workflow:#})
from_readme=(${from_readme:#})

if (( ${#from_workflow} == 0 )); then
  print -u2 "zd digests: no qualified images found in $workflow"
  exit 1
fi
if (( ${#from_readme} == 0 )); then
  print -u2 "zd digests: no qualified images found in $readme"
  exit 1
fi

if [[ "${(F)from_workflow}" != "${(F)from_readme}" ]]; then
  print -u2 "zd digests: workflow and README differ (the workflow is authoritative)"
  print -u2 "workflow ($workflow):"
  print -u2 -l -- "  ${^from_workflow[@]}"
  print -u2 "README ($readme):"
  print -u2 -l -- "  ${^from_readme[@]}"
  exit 1
fi

print "zd digests: ${#from_workflow} qualified image(s) match in workflow and README"
