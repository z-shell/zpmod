#!/usr/bin/env zsh
# Validate custom_dot: '.' and 'source' should still source a file correctly with zpmod loaded
set -euo pipefail
emulate -L zsh

# Source helpers and set test name
source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="builtin/custom_dot"

# Load module
load_zpmod

# Create a temp dir with a file to source
workdir=$(mktemp -d)
trap 'rm -rf -- "$workdir"' EXIT

cat > "$workdir/foo.zsh" <<'EOS'
print -r -- "hello from foo"
export FOO_VAR=ok
EOS

# Use '.' and 'source' and capture effects
out1=$(. "$workdir/foo.zsh")
[[ $out1 == "hello from foo" ]]
[[ ${FOO_VAR:-} == ok ]]

unset FOO_VAR
out2=$(source "$workdir/foo.zsh")
[[ $out2 == "hello from foo" ]]
[[ ${FOO_VAR:-} == ok ]]

# Also source via a ./ relative path to exercise path trimming logic
unset FOO_VAR
(
	cd "$workdir"
	out3=$(. "./foo.zsh")
	[[ $out3 == "hello from foo" ]]
	[[ ${FOO_VAR:-} == ok ]]
)

print -r -- "custom_dot OK"

# Report
test_status "PASS" "$TEST_NAME"
test_info "custom_dot test completed successfully"
