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

cleanup() {
  rm -rf -- "$tmp_dir"
}
trap cleanup EXIT

mkdir -p "$mock_bin"
cat > "$mock_bin/gh" <<'MOCK_GH'
#!/usr/bin/env zsh
emulate -R zsh
setopt err_exit no_unset pipe_fail

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
print -r -- "$MOCK_RELEASES_JSON"
MOCK_GH
chmod +x "$mock_bin/gh"

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

print -r -- "${MOCK_JQ_OUTPUT-}"
MOCK_JQ
  chmod +x "$mock_bin/jq"
fi

fail_test() {
  print -ru2 -- "$*"
  exit 1
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
export MOCK_RELEASES_JSON MOCK_JQ_OUTPUT
run_resolver || fail_test 'one exact draft did not resolve successfully'
[[ $(< "$stdout_file") == 377741043 ]] ||
  fail_test 'resolver did not return the exact numeric release ID'

MOCK_RELEASES_JSON='[[
  {"id": 11, "draft": false, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"},
  {"id": 12, "draft": true, "tag_name": "v2.0.5", "target_commitish": "39b02adf471d1f40df0b985565dc16b97add1127"}
]]'
MOCK_JQ_OUTPUT=
export MOCK_RELEASES_JSON MOCK_JQ_OUTPUT
if run_resolver; then
  fail_test 'resolver accepted zero exact drafts'
fi
grep -q 'found 0' "$stderr_file" ||
  fail_test 'zero-match failure did not report its cardinality'

MOCK_RELEASES_JSON='[[
  {"id": 21, "draft": true, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"},
  {"id": 22, "draft": true, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"}
]]'
MOCK_JQ_OUTPUT=$'21\n22'
export MOCK_RELEASES_JSON MOCK_JQ_OUTPUT
if run_resolver; then
  fail_test 'resolver accepted multiple exact drafts'
fi
grep -q 'found 2' "$stderr_file" ||
  fail_test 'multiple-match failure did not report its cardinality'

MOCK_RELEASES_JSON='[[
  {"id": "not-numeric", "draft": true, "tag_name": "v2.0.5", "target_commitish": "ea511d977800e3916776ad43d055afd4dcca734f"}
]]'
MOCK_JQ_OUTPUT=not-numeric
export MOCK_RELEASES_JSON MOCK_JQ_OUTPUT
if run_resolver; then
  fail_test 'resolver accepted a non-numeric release ID'
fi
grep -q 'not numeric' "$stderr_file" ||
  fail_test 'non-numeric failure was not explicit'

MOCK_RELEASES_JSON='[]'
MOCK_GH_STATUS=73
export MOCK_RELEASES_JSON MOCK_GH_STATUS
if run_resolver; then
  fail_test 'resolver hid a GitHub API failure'
fi
grep -q 'mock GitHub API failure' "$stderr_file" ||
  fail_test 'GitHub API failure output was not preserved'

print -r -- 'release_draft_resolution OK'
