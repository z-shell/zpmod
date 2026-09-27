#!/usr/bin/env bash
# Exercise Zi's existing source-build integration with pinned inputs.
set -euo pipefail
: "${ZSH_EXECUTABLE:?}" "${RUNNER_TEMP:?}"
zi_revision=67fbb057fe05bc5664c0e558185a18b34a81defb
candidate_revision=$(git rev-parse HEAD)
integration_root=$(mktemp -d "${RUNNER_TEMP}/zpmod-zi.XXXXXX")
zi_home="${integration_root}/home"
zi_data="${integration_root}/data"
candidate_remote="${integration_root}/zpmod-candidate.git"
mkdir -p "${zi_home}" "${zi_data}/zi/zmodules" qualification-results
git init --quiet "${zi_data}/zi/bin"
git -C "${zi_data}/zi/bin" fetch --depth 1 https://github.com/z-shell/zi.git "${zi_revision}"
git -C "${zi_data}/zi/bin" checkout --quiet --detach FETCH_HEAD
actual_zi=$(git -C "${zi_data}/zi/bin" rev-parse HEAD)
[[ ${actual_zi} == "${zi_revision}" ]]
git init --bare --quiet "${candidate_remote}"
git push --quiet "${candidate_remote}" HEAD:refs/heads/ci-candidate
git --git-dir="${candidate_remote}" symbolic-ref HEAD refs/heads/ci-candidate
git clone --quiet "${candidate_remote}" "${zi_data}/zi/zmodules/zpmod"
# Zsh expands its own parameters in the child.
# shellcheck disable=SC2016
HOME="${zi_home}" ZDOTDIR="${zi_home}" XDG_DATA_HOME="${zi_data}" \
  XDG_CACHE_HOME="${zi_home}/.cache" "${ZSH_EXECUTABLE}" -f -c '
    source "$XDG_DATA_HOME/zi/bin/zi.zsh" || exit 1
    zi module build --from-source
  '
module_root="${zi_data}/zi/zmodules/zpmod"
actual_candidate=$(git -C "${module_root}" rev-parse HEAD)
[[ ${actual_candidate} == "${candidate_revision}" ]]
test -s "${module_root}/Src/zi/zpmod.so"
# Zsh expands its own parameters in the child.
# shellcheck disable=SC2016
HOME="${zi_home}" ZDOTDIR="${zi_home}" XDG_DATA_HOME="${zi_data}" \
  ZPMOD_MODULE_ROOT="${module_root}" "${ZSH_EXECUTABLE}" -f -c '
    emulate -R zsh
    module_path=( "$ZPMOD_MODULE_ROOT/Src" $module_path )
    zmodload zi/zpmod || exit 1
    [[ $(whence -w zpmod) == *builtin* ]] || exit 1
    print -r -- : > "$HOME/fixture.zsh" || exit 1
    source "$HOME/fixture.zsh" || exit 1
    zpmod source-study --json
  ' >"${integration_root}/source-study.json"
python3 - "${integration_root}/source-study.json" "${zi_revision}" "${candidate_revision}" <<'PY'
import json
import sys
from pathlib import Path
report = json.loads(Path(sys.argv[1]).read_text())
assert report['schema_version'] == 1 and report['status'] == 'complete'
assert len(report['events']) == 1 and report['events'][0]['exit_status'] == 0
Path('qualification-results/zi.json').write_text(json.dumps({
    'zi_revision': sys.argv[2], 'zpmod_revision': sys.argv[3],
    'source_study': 'passed', 'integration': 'passed'
}, indent=2) + '\n')
PY
