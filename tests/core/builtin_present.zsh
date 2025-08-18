#!/usr/bin/env zsh
# Verify that the 'zpmod' builtin is available after loading the module

set -euo pipefail
emulate -L zsh

# Source test helpers and set test name from tests root
source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="zpmod_builtin_present"

# Load module via helper
load_zpmod

# Verify builtin is present
assert_builtin_exists "zpmod" "zpmod builtin not found after loading"

# Optionally check help flag works
zpmod -h >/dev/null 2>&1 || true

print -r -- "zpmod builtin present"

# Report
test_status "PASS" "$TEST_NAME"
test_info "builtin_present test completed successfully"
