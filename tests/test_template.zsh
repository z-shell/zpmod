#!/usr/bin/env zsh
# Template for new zpmod tests
# Copy this file and modify for new test cases

set -euo pipefail
emulate -L zsh

# Source test helpers (works from any subfolder)
source "${0:A:h:h}/test_helpers.zsh"

# Set test name for reporting (folder/name)
TEST_NAME="${${0:A:h:t}}/${${0:t}%.zsh}"

# Load zpmod module
load_zpmod

# Test description: Replace with actual test description
test_info "Running $TEST_NAME - [Description of what this test does]"

#
# Test implementation goes here
#

# Example test structure:
#
# # Setup test data
# test_debug "Setting up test data"
#
# # Execute the functionality being tested
# test_debug "Testing [specific functionality]"
#
# # Verify results with assertions
# assert_equal "$actual_result" "$expected_result" "Result should match expectation"
# assert_contains "$output" "expected_string" "Output should contain expected text"
# assert_file_exists "/path/to/file" "Output file should be created"
#
# # Test passed
# test_status "PASS" "$TEST_NAME"

# Replace with actual test implementation
test_info "This is a template - replace with actual test code"
skip_test "Template test - not implemented"

# vim:ft=zsh:et:sts=2:sw=2
