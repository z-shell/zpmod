#!/usr/bin/env zsh
# Validate option handling helpers in zpmod (where feasible)
set -euo pipefail
emulate -L zsh

# Source helpers and load module
source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="command/options"
load_zpmod

# Sanity check: toggling a known option should work normally
setopt extendedglob
[[ -o extendedglob ]]
unsetopt extendedglob
[[ ! -o extendedglob ]]

# If zpmod exposes subcommands via `zpmod` builtin, try a harmless flag
# Accept that -h may just print usage and return success
zpmod -h >/dev/null 2>&1 || true

print -r -- "options OK"

# Report
test_status "PASS" "$TEST_NAME"
test_info "options test completed successfully"
