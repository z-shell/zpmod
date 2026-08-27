#!/usr/bin/env zsh
# Regression tests for resolving one exact private release draft.
emulate -R zsh
setopt err_exit no_unset pipe_fail

repo_root=${0:A:h:h:h}
resolver="$repo_root/scripts/resolve-release-draft.zsh"
tmp_dir=$(mktemp -d)
mock_bin="$tmp_dir/bin"
stdout_file="$tmp_dir/stdout"
stderr_file="$tmp_dir/stderr"
gh_count_file="$tmp_dir/gh-count"
sleep_count_file="$tmp_dir/sleep-count"

cleanup() {
  rm -rf -- "$tmp_dir"
}
trap cleanup EXIT

mkdir -p "$mock_bin"
cat > "$mock_bin/gh" <<'MOCK_GH'
#!/usr/bin/env zsh
emulate -R zsh
setopt err_exit no_unset pipe_fail

typeset -i invocation_count=0
if [[ -s $MOCK_GH_COUNT_FILE ]]; then
  invocation_count=$(< "$MOCK_GH_COUNT_FILE")
fi
(( ++invocation_count ))
print -r -- "$invocation_count" >| "$MOCK_GH_COUNT_FILE"

if (( ${MOCK_GH_STATUS:-0} != 0 )); then
  print -ru2 -- 'mock GitHub API failure'
  exit "$MOCK_GH_STATUS"
fi

if (( $# != 4 )) ||
  [[ $1 != api || $2 != --paginate || $3 != --slurp ]] ||
  [[ $4 != 'repos/z-shell/zpmod/releases?per_page=100' ]]; then
  print -ru2 -- "unexpected gh invocation: $*"
  exit 64
fi
if (( invocation_count <= ${MOCK_GH_ZERO_RESPONSES:-0} )); then
  print -r -- '[[]]'
  exit 0
fi
print -r -- "$MOCK_RELEASES_JSON"
MOCK_GH
chmod +x "$mock_bin/gh"

cat > "$mock_bin/sleep" <<'MOCK_SLEEP'
#!/usr/bin/env zsh
emulate -R zsh
setopt err_exit no_unset pipe_fail

if (( $# != 1 )) || [[ $1 != "$MOCK_EXPECTED_SLEEP_SECONDS" ]]; then
  print -ru2 -- "unexpected sleep invocation: $*"
  exit 64
fi

typeset -i invocation_count=0
if [[ -s $MOCK_SLEEP_COUNT_FILE ]]; then
  invocation_count=$(< "$MOCK_SLEEP_COUNT_FILE")
fi
(( ++invocation_count ))
print -r -- "$invocation_count" >| "$MOCK_SLEEP_COUNT_FILE"
MOCK_SLEEP
chmod +x "$mock_bin/sleep"

if ! command -v jq >/dev/null 2>&1; then
  cat > "$mock_bin/jq" <<'MOCK_JQ'
#!/usr/bin/env zsh
emulate -R zsh
setopt err_exit no_unset pipe_fail

if (( $# != 8 )) ||
  [[ $1 != -r || $2 != --arg || $3 != tag ]] ||
  [[ $4 != v2.0.5 || $5 != --arg || $6 != commit ]] ||
  [[ $7 != ea511d977800e3916776ad43d055afd4dcca734f ]] ||
  [[ $8 != *'.draft == true'* ]] ||
  [[ $8 != *'.tag_name == $tag'* ]] ||
  [[ $8 != *'.target_commitish == $commit'* ]]; then
  print -ru2 -- "unexpected jq invocation: $*"
  exit 64
fi

typeset releases_json
releases_json=$(<&0)
[[ $releases_json == '[[]]' ]] && exit 0
print -r -- "${MOCK_JQ_OUTPUT-}"
MOCK_JQ
  chmod +x "$mock_bin/jq"
fi

fail_test() {
  print -ru2 -- "$*"
  exit 1
}

reset_mock_counts() {
  : >| "$gh_count_file"
  : >| "$sleep_count_file"
}

mock_count() {
  if [[ -s $1 ]]; then
    print -r -- "$(< "$1")"
  else
    print -r -- 0
  fi
}

run_resolver() {
  PATH="$mock_bin:$PATH" zsh -f "$resolver" \
    z-shell/zpmod v2.0.5 ea511d977800e3916776ad43d055afd4dcca734f \
    > "$stdout_file" 2> "$stderr_file"
}

MOCK_RELEASES_JSON='[
[
  {"id": 11, "draft": false, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"},
  {"id": 12, "draft": true, "tag_name": "v2.0.5", "target_commitish": "39b02adf471d1f40df0b985565dc16b97add1127"}
],
[
  {"id": 377741043, "draft": true, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"}
]
]'
MOCK_JQ_OUTPUT=377741043
MOCK_GH_STATUS=0
MOCK_GH_ZERO_RESPONSES=0
MOCK_EXPECTED_SLEEP_SECONDS=0
ZPMOD_RELEASE_DRAFT_MAX_ATTEMPTS=3
ZPMOD_RELEASE_DRAFT_DELAY_SECONDS=0
export MOCK_RELEASES_JSON MOCK_JQ_OUTPUT MOCK_GH_STATUS
export MOCK_GH_ZERO_RESPONSES MOCK_GH_COUNT_FILE="$gh_count_file"
export MOCK_EXPECTED_SLEEP_SECONDS MOCK_SLEEP_COUNT_FILE="$sleep_count_file"
export ZPMOD_RELEASE_DRAFT_MAX_ATTEMPTS ZPMOD_RELEASE_DRAFT_DELAY_SECONDS
reset_mock_counts
run_resolver || fail_test 'one exact draft did not resolve successfully'
[[ $(< "$stdout_file") == 377741043 ]] ||
  fail_test 'resolver did not return the exact numeric release ID'
[[ $(mock_count "$gh_count_file") == 1 ]] ||
  fail_test 'immediately visible draft was queried more than once'
[[ $(mock_count "$sleep_count_file") == 0 ]] ||
  fail_test 'immediately visible draft caused an unnecessary delay'

MOCK_GH_ZERO_RESPONSES=2
export MOCK_GH_ZERO_RESPONSES
reset_mock_counts
run_resolver || fail_test 'delayed exact draft did not resolve successfully'
[[ $(< "$stdout_file") == 377741043 ]] ||
  fail_test 'delayed resolver did not return the exact numeric release ID'
[[ $(mock_count "$gh_count_file") == 3 ]] ||
  fail_test 'delayed resolver did not stop at the first exact match'
[[ $(mock_count "$sleep_count_file") == 2 ]] ||
  fail_test 'delayed resolver did not pause only between zero-match attempts'

MOCK_RELEASES_JSON='[[
  {"id": 11, "draft": false, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"},
  {"id": 12, "draft": true, "tag_name": "v2.0.5", "target_commitish": "39b02adf471d1f40df0b985565dc16b97add1127"}
]]'
MOCK_JQ_OUTPUT=
MOCK_GH_ZERO_RESPONSES=0
export MOCK_RELEASES_JSON MOCK_JQ_OUTPUT MOCK_GH_ZERO_RESPONSES
reset_mock_counts
if run_resolver; then
  fail_test 'resolver accepted zero exact drafts'
fi
grep -q 'found 0' "$stderr_file" ||
  fail_test 'zero-match failure did not report its cardinality'
grep -q 'after 3 attempts' "$stderr_file" ||
  fail_test 'zero-match failure did not report the polling bound'
[[ $(mock_count "$gh_count_file") == 3 ]] ||
  fail_test 'zero-match resolver did not exhaust its bounded attempts'
[[ $(mock_count "$sleep_count_file") == 2 ]] ||
  fail_test 'zero-match resolver did not pause only between attempts'

MOCK_RELEASES_JSON='[[
  {"id": 21, "draft": true, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"},
  {"id": 22, "draft": true, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"}
]]'
MOCK_JQ_OUTPUT=$'21\n22'
export MOCK_RELEASES_JSON MOCK_JQ_OUTPUT
reset_mock_counts
if run_resolver; then
  fail_test 'resolver accepted multiple exact drafts'
fi
grep -q 'found 2' "$stderr_file" ||
  fail_test 'multiple-match failure did not report its cardinality'
[[ $(mock_count "$gh_count_file") == 1 ]] ||
  fail_test 'multiple-match resolver retried an unsafe cardinality'
[[ $(mock_count "$sleep_count_file") == 0 ]] ||
  fail_test 'multiple-match resolver delayed before failing'

MOCK_RELEASES_JSON='[[
  {"id": "not-numeric", "draft": true, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"}
]]'
MOCK_JQ_OUTPUT=not-numeric
export MOCK_RELEASES_JSON MOCK_JQ_OUTPUT
reset_mock_counts
if run_resolver; then
  fail_test 'resolver accepted a non-numeric release ID'
fi
grep -q 'not numeric' "$stderr_file" ||
  fail_test 'non-numeric failure was not explicit'
[[ $(mock_count "$gh_count_file") == 1 ]] ||
  fail_test 'resolver retried a non-numeric release ID'
[[ $(mock_count "$sleep_count_file") == 0 ]] ||
  fail_test 'resolver delayed after a non-numeric release ID'

MOCK_RELEASES_JSON='[]'
MOCK_GH_STATUS=73
export MOCK_RELEASES_JSON MOCK_GH_STATUS
reset_mock_counts
if run_resolver; then
  fail_test 'resolver hid a GitHub API failure'
fi
grep -q 'mock GitHub API failure' "$stderr_file" ||
  fail_test 'GitHub API failure output was not preserved'
[[ $(mock_count "$gh_count_file") == 1 ]] ||
  fail_test 'resolver retried a GitHub API failure'
[[ $(mock_count "$sleep_count_file") == 0 ]] ||
  fail_test 'resolver delayed after a GitHub API failure'

MOCK_GH_STATUS=0
ZPMOD_RELEASE_DRAFT_MAX_ATTEMPTS=0
export MOCK_GH_STATUS ZPMOD_RELEASE_DRAFT_MAX_ATTEMPTS
reset_mock_counts
if run_resolver; then
  fail_test 'resolver accepted a zero attempt bound'
fi
grep -q 'must be a positive integer' "$stderr_file" ||
  fail_test 'invalid attempt bound failure was not explicit'
[[ $(mock_count "$gh_count_file") == 0 ]] ||
  fail_test 'resolver queried GitHub with an invalid attempt bound'

ZPMOD_RELEASE_DRAFT_MAX_ATTEMPTS=3
ZPMOD_RELEASE_DRAFT_DELAY_SECONDS=invalid
export ZPMOD_RELEASE_DRAFT_MAX_ATTEMPTS ZPMOD_RELEASE_DRAFT_DELAY_SECONDS
reset_mock_counts
if run_resolver; then
  fail_test 'resolver accepted a non-numeric delay'
fi
grep -q 'must be a non-negative integer' "$stderr_file" ||
  fail_test 'invalid retry delay failure was not explicit'
[[ $(mock_count "$gh_count_file") == 0 ]] ||
  fail_test 'resolver queried GitHub with an invalid retry delay'

print -r -- 'release_draft_resolution OK'
