# zpmod Testing Guide

This guide describes the testing infrastructure and conventions for the zpmod project.

## Overview

The zpmod project uses a lightweight, dependency-free testing approach based on:

- zsh test scripts for test implementation
- CMake/CTest for test discovery and execution
- Enhanced test helpers for better assertions and reporting
- Zero external dependencies beyond zsh and CMake

## Test Structure

### Files and Folders

All tests are under `tests/`:

- `test_helpers.zsh`: Shared assertion functions and utilities
- `test_template.zsh`: Template for creating new tests
- Suites in subfolders: `core/`, `builtin/`, `command/`, `filesystem/`, `file_io/`, `platform/`, `benchmark/`

### Naming

- Descriptive names: `zpreadarray.zsh`, `zpdirlist.zsh`, etc.
- Lowercase with underscores
- `.zsh` extension
- Shebang: `#!/usr/bin/env zsh`

### Categories

- `core/`: Smoke and core availability
- `builtin/`: Builtins like zpreadarray, custom_dot
- `command/`: `zpmod` CLI and subcommands
- `filesystem/`: Directory listing and path stat helpers
- `file_io/`: File reading and parsing
- `platform/`: OS-specific behaviors
- `benchmark/`: Benchmark output and renderer integrity, without timing thresholds

## Writing Tests

### Basic Structure

```zsh
#!/usr/bin/env zsh
# Test description

set -euo pipefail
emulate -L zsh

# Source test helpers
source "${0:A:h}/test_helpers.zsh"

# Set test name for reporting
TEST_NAME="${${0:t}%.zsh}"

# Test description
test_info "Running $TEST_NAME - Description of test"

# Load zpmod module
load_zpmod

# Test implementation
test_debug "Testing specific functionality"
# ... test code here ...

# Assertions
assert_equal "$actual" "$expected" "Values should match"
assert_contains "$output" "expected text" "Output should contain text"

# Test completion
test_status "PASS" "$TEST_NAME"
```

### Assertions

#### Value comparisons

- `assert_equal actual expected [message]`
- `assert_not_equal actual unexpected [message]`
- `assert_contains haystack needle [message]`
- `assert_not_contains haystack needle [message]`
- `assert_empty value [message]`
- `assert_not_empty value [message]`

#### File system helpers

- `assert_dir_exists path [message]`
- `assert_greater_than actual expected [message]`
- `assert_less_than actual expected [message]`

#### Arrays

- `assert_array_size array_name expected_size [message]`

#### Commands

- `assert_success "command" [message]`
- `assert_failure "command" [message]`

Notes:

- `assert_equal` on multi-line values prints a unified diff (requires `diff`).
- `assert_builtin_exists builtin_name [message]` verifies builtin availability.

## Utilities

### Status and reporting

```cmake
add_test(NAME zpmod_my_new_test
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/builtin/my_new_test.zsh)
set_tests_properties(zpmod_my_new_test PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")
```

- `test_status "PASS|FAIL|SKIP" "test_name"`
- `skip_test "reason"`
- `pass_test [message]`

### Environment variables

```bash
cd tests/builtin
ZPMOD_STAGE_MODULE_DIR=../../build-cmake/stage/lib/zsh zsh my_new_test.zsh
```

- `ZPMOD_TEST_START_TIME` — automatic (used for timing)

```bash
ctest --test-dir build-cmake -R zpmod_my_new_test --output-on-failure
```

## Running Tests

### Individual tests

```bash
# Example
zsh tests/core/smoke.zsh

# With debug output
ZPMOD_TEST_DEBUG=1 zsh tests/builtin/zpreadarray.zsh
```

### All tests via CMake/CTest

```bash
# Configure and build
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake

# Run all tests
ctest --test-dir build-cmake --output-on-failure

# Run specific test
ctest --test-dir build-cmake -R zpmod_smoke --output-on-failure

# Run in parallel
ctest --test-dir build-cmake -j 4
```

### Labeled runs

```bash
ctest --test-dir build-cmake -L core --output-on-failure
ctest --test-dir build-cmake -L filesystem --output-on-failure
```

### Debug output on failure

```bash
ZPMOD_TEST_DEBUG=1 ctest --test-dir build-cmake -L file_io --output-on-failure
```

In CI, failed label jobs are automatically re-run with `ZPMOD_TEST_DEBUG=1`.

### Colored output and context

```bash
ZPMOD_TEST_COLOR=1 ctest --test-dir build-cmake -L file_io --output-on-failure

# Local single test run with detailed context
ZPMOD_STAGE_MODULE_DIR=../build-cmake/stage/lib/zsh \
ZPMOD_TEST_COLOR=always ZPMOD_TEST_DEBUG=1 \
zsh tests/file_io/zpreadfile.zsh
```

## CI/CD

- Runs on PRs and pushes
- Multiple platforms (Linux, macOS, Windows)
- Different build configurations

## Adding New Tests

1. Copy the template:

   ```bash
   cp tests/test_template.zsh tests/my_new_test.zsh
   ```

2. Update the test:
   - Replace description and test name
   - Implement test logic
   - Add appropriate assertions
   - Remove template placeholder code

3. Register with CMake:

   ```cmake
   add_test(NAME zpmod_my_new_test
     COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/my_new_test.zsh)
   set_tests_properties(zpmod_my_new_test PROPERTIES
     ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")
   ```

4. Test your test:

   ```bash
   # Run locally first
   cd tests
   ZPMOD_STAGE_MODULE_DIR=../build-cmake/stage/lib/zsh zsh my_new_test.zsh

   # Then via CTest
   ctest --test-dir build-cmake -R zpmod_my_new_test
   ```

## Best Practices

### Design

- One concept per test
- Clear names
- Helpful failure messages
- Independent tests

### Error handling

- Use `set -euo pipefail`
- Prefer helpers for consistent errors
- Provide meaningful context

### Performance

- Keep tests fast
- Use `test_debug` for optional verbosity
- Clean up temp files/resources

### Debugging

- Use `test_debug`
- Set `ZPMOD_TEST_DEBUG=1`
- Include relevant context

## Troubleshooting

### Common issues

Module not found:

- Ensure `ZPMOD_STAGE_MODULE_DIR` is set
- Verify the module is staged
- Check the staged module exists

Test failures:

- Run with `ZPMOD_TEST_DEBUG=1`
- Ensure dependencies exist
- Verify environment expectations

Permission errors:

- Ensure test files are executable
- Check build dir permissions

### Getting help

- See existing tests
- Review `test_helpers.zsh`
- Consult project docs
- Open an issue

## Future Improvements

- Test coverage reporting
- Faster/parallelized execution
- CI improvements
- Cross-release benchmark trend views after multiple comparable result sets exist
- Integration test categories

---

This testing approach provides a robust, maintainable foundation for ensuring zpmod quality while keeping the infrastructure simple and
dependency-free.
