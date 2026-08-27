#!/usr/bin/env zsh
# Keep the documented compatibility floor aligned with its executable CI gates.
emulate -R zsh
setopt err_exit no_unset pipe_fail

typeset repo_root=${0:A:h:h:h}
typeset minimum_zsh=5.8.1
typeset dockerfile="$repo_root/docker/minimal.Dockerfile"
typeset workflow="$repo_root/.github/workflows/compatibility.yml"
typeset compatibility_doc="$repo_root/docs/reference/compatibility.md"

fail_test() {
  print -ru2 -- "$*"
  exit 1
}

grep -Fq "ARG ZPMOD_MINIMUM_ZSH=$minimum_zsh" "$dockerfile" ||
  fail_test "minimum container does not pin Zsh $minimum_zsh"
grep -Fq 'actual_version=' "$dockerfile" ||
  fail_test 'minimum container does not inspect the installed Zsh version'
grep -Fq 'ZPMOD_MINIMUM_ZSH' "$dockerfile" ||
  fail_test 'minimum container does not enforce its declared Zsh version'

[[ -f $workflow ]] || fail_test 'scheduled compatibility workflow is missing'
grep -Fq 'schedule:' "$workflow" ||
  fail_test 'compatibility workflow is not scheduled'
grep -Fq 'zi module build --from-source' "$workflow" ||
  fail_test 'compatibility workflow does not build the candidate through Zi'
grep -Fq 'ZI_REVISION=' "$workflow" ||
  fail_test 'compatibility workflow does not record the Zi revision'
grep -Fq 'ZPMOD_EXPECTED_REVISION=' "$workflow" ||
  fail_test 'compatibility workflow does not preserve the candidate revision'
grep -Fq 'zmodload -i zi/zpmod' "$workflow" ||
  fail_test 'compatibility workflow does not load the Zi-managed module'

[[ -f $compatibility_doc ]] || fail_test 'compatibility reference is missing'
grep -Fq "minimum supported Zsh release is **$minimum_zsh**" \
  "$compatibility_doc" ||
  fail_test "documentation does not declare Zsh $minimum_zsh as the floor"
grep -Fq 'does not promise a universal Zsh module ABI' \
  "$compatibility_doc" ||
  fail_test 'documentation does not define the prebuilt-package ABI boundary'

print -r -- 'compatibility_policy OK'
