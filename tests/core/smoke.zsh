#!/usr/bin/env zsh
# Minimal smoke test: ensure zpmod loads from the staged module_path.

set -euo pipefail
emulate -L zsh

# Source test helpers for enhanced testing (from tests root)
source "${0:A:h:h}/test_helpers.zsh"

# Set test name for reporting
TEST_NAME="zpmod_smoke"

# Test description
test_info "Running smoke test - verifying zpmod module loads correctly"

# Load zpmod module using helper function
load_zpmod

# Verify loaded by checking builtin presence
test_debug "Verifying zpmod builtin is available"
assert_builtin_exists "zpmod" "zpmod builtin should be available after loading"

# Verify custom_dot builtin if enabled
if whence -w custom_dot >/dev/null 2>&1; then
	test_debug "custom_dot builtin detected and available"
fi

# Test passed
test_status "PASS" "$TEST_NAME"
test_info "zpmod smoke test completed successfully"
