# Enhanced zpmod test suite with improved reporting and organization

if(NOT ZSH_EXECUTABLE)
  message(STATUS "ZSH not found - skipping tests")
  return()
endif()

# Test configuration
set(ZPMOD_STAGE_MODULE_DIR "${ZPMOD_STAGE_DIR}/${ZPMOD_ZSH_MODDIR}")

# Test categories for organization
set(ZPMOD_TEST_LABELS "")

# Ensure the staged module exists before running any tests when invoking ctest directly.
# We create a CTest fixture that builds the 'stage' target once and have tests require it.
add_test(NAME stage_fixture
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target stage)
set_tests_properties(stage_fixture PROPERTIES FIXTURES_SETUP "StageReady" TIMEOUT 120)

# Helper function to add zpmod tests with consistent properties
function(add_zpmod_test TEST_NAME TEST_FILE)
  set(options SLOW)
  set(oneValueArgs TIMEOUT CATEGORY)
  set(multiValueArgs LABELS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Add the test
  add_test(NAME ${TEST_NAME}
    COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE})

  # Set standard properties
  set_tests_properties(${TEST_NAME} PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}"
  FIXTURES_REQUIRED "StageReady")

  # Set timeout (default 30 seconds, 60 for slow tests)
  if(ARG_SLOW)
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 60)
  elseif(ARG_TIMEOUT)
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT ${ARG_TIMEOUT})
  else()
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 30)
  endif()

  # Set category and labels
  set(test_labels "")
  if(ARG_CATEGORY)
    list(APPEND test_labels ${ARG_CATEGORY})
  endif()
  if(ARG_LABELS)
    list(APPEND test_labels ${ARG_LABELS})
  endif()

  if(test_labels)
    set_tests_properties(${TEST_NAME} PROPERTIES LABELS "${test_labels}")
    list(APPEND ZPMOD_TEST_LABELS ${test_labels})
    list(REMOVE_DUPLICATES ZPMOD_TEST_LABELS)
    set(ZPMOD_TEST_LABELS ${ZPMOD_TEST_LABELS} PARENT_SCOPE)
  endif()
endfunction()

# Minimal ztst adapter runner (optional)
function(add_zpmod_ztst TEST_NAME ZTST_FILE)
  # Use zsh to run our minimal adapter with the ztst script as argument
  add_test(NAME ${TEST_NAME}
    COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/ztst/ztst_mini.zsh ${CMAKE_CURRENT_SOURCE_DIR}/ztst/${ZTST_FILE})

  # Pass stage dir
  set_tests_properties(${TEST_NAME} PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}"
  FIXTURES_REQUIRED "StageReady")

  # Reasonable timeout and label as 'ztst'
  set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 30 LABELS "ztst;compat")
endfunction()

# Core functionality tests
add_zpmod_test(zpmod_smoke core/smoke.zsh
  CATEGORY "core" LABELS "smoke;essential")

add_zpmod_test(zpmod_builtin_present core/builtin_present.zsh
  CATEGORY "core" LABELS "builtin;essential")

# Custom dot functionality
add_zpmod_test(zpmod_custom_dot builtin/custom_dot.zsh
  CATEGORY "builtin" LABELS "custom_dot")

# Source-study formatting and options
add_zpmod_test(zpmod_source_study core/source_study.zsh
  CATEGORY "core" LABELS "source-study")

# readarray builtin tests
add_zpmod_test(zpmod_readarray builtin/readarray.zsh
  CATEGORY "builtin" LABELS "readarray;comprehensive")

add_zpmod_test(zpmod_readarray_delim builtin/readarray_delim.zsh
  CATEGORY "builtin" LABELS "readarray;edge_cases")

add_zpmod_test(zpmod_readarray_fd builtin/readarray_fd.zsh
  CATEGORY "builtin" LABELS "readarray;file_descriptor")

add_zpmod_test(zpmod_readarray_clear builtin/readarray_clear.zsh
  CATEGORY "builtin" LABELS "readarray;behavior")

add_zpmod_test(zpmod_readarray_large builtin/readarray_large.zsh
  CATEGORY "builtin" LABELS "readarray;stress" SLOW)

add_zpmod_test(zpmod_readarray_fd_t builtin/readarray_fd_t.zsh
  CATEGORY "builtin" LABELS "readarray;file_descriptor;trim")

# zpmod command and subcommand tests
add_zpmod_test(zpmod_report_append command/zpmod_report_append.zsh
  CATEGORY "command" LABELS "report;memory_safety")

add_zpmod_test(zpmod_options command/options.zsh
  CATEGORY "command" LABELS "options;argument_parsing")

add_zpmod_test(zpmod_subcommands command/zpmod_subcommands.zsh
  CATEGORY "command" LABELS "subcommands;cli")

# PATH warmup subcommand (performance helper)
add_zpmod_test(zpmod_path_warmup command/zpmod_path_warmup.zsh
  CATEGORY "command" LABELS "performance;path")

add_zpmod_test(zpmod_path_warmup_prune command/zpmod_path_warmup_prune.zsh
  CATEGORY "command" LABELS "performance;path")

# FPATH index generation & preload behavior
add_zpmod_test(zpmod_fpath_index command/zpmod_fpath_index.zsh
  CATEGORY "command" LABELS "fpath;indexing")

# Compat: options mapping sentinel and basics
add_zpmod_test(zpmod_options_mapping compat/options_mapping.zsh
  CATEGORY "core" LABELS "compat;options;loader")

# Filesystem utility tests
add_zpmod_test(zpmod_zpdirlist filesystem/zpdirlist.zsh
  CATEGORY "filesystem" LABELS "directory;listing")

add_zpmod_test(zpmod_zpdirlist_filters filesystem/zpdirlist_filters.zsh
  CATEGORY "filesystem" LABELS "directory;filtering")

add_zpmod_test(zpmod_zppathstat filesystem/zppathstat.zsh
  CATEGORY "filesystem" LABELS "stat;path_info")

add_zpmod_test(zpmod_zppathstat_fields filesystem/zppathstat_fields.zsh
  CATEGORY "filesystem" LABELS "stat;field_access")

add_zpmod_test(zpmod_zppathstat_follow filesystem/zppathstat_follow.zsh
  CATEGORY "filesystem" LABELS "stat;symlink_follow")

add_zpmod_test(zpmod_zppathstat_errno_fields filesystem/zppathstat_errno_fields.zsh
  CATEGORY "filesystem" LABELS "stat;error_handling")

# File reading tests
add_zpmod_test(zpmod_zpreadfile file_io/zpreadfile.zsh
  CATEGORY "file_io" LABELS "read;basic")

add_zpmod_test(zpmod_zpreadfile_errors file_io/zpreadfile_errors.zsh
  CATEGORY "file_io" LABELS "read;error_handling")

add_zpmod_test(zpmod_zpreadfile_crlf file_io/zpreadfile_crlf.zsh
  CATEGORY "file_io" LABELS "read;line_endings")

add_zpmod_test(zpmod_zpreadfile_trailing_delim file_io/zpreadfile_trailing_delim.zsh
  CATEGORY "file_io" LABELS "read;delimiters")

add_zpmod_test(zpmod_zpreadfile_mixed_line_endings file_io/zpreadfile_mixed_line_endings.zsh
  CATEGORY "file_io" LABELS "read;line_endings;edge_cases")

add_zpmod_test(zpmod_zpreadfile_empty file_io/zpreadfile_empty.zsh
  CATEGORY "file_io" LABELS "read;empty_files;edge_cases")

add_zpmod_test(zpmod_zpreadfile_zero file_io/zpreadfile_zero.zsh
  CATEGORY "file_io" LABELS "read;null_bytes;edge_cases")

# Platform-specific tests
add_zpmod_test(zpmod_macos_suffix platform/macos_suffix.zsh
  CATEGORY "platform" LABELS "macos;suffix;loader" TIMEOUT 15)

# Optional: small ztst suite for internals-heavy checks
add_zpmod_ztst(zpmod_ztst_options_mapping options_mapping.ztst)
add_zpmod_ztst(zpmod_ztst_param_hash param_hash_semantics.ztst)
add_zpmod_ztst(zpmod_ztst_sigcount sigcount_shim.ztst)
add_zpmod_ztst(zpmod_ztst_emulate_matrix emulate_setopt_matrix.ztst)

# Include-order enforcement as a CTest (lint-style check)
add_test(NAME include_order_check
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_SOURCE_DIR}/scripts/check_include_order.zsh)
set_tests_properties(include_order_check PROPERTIES
  TIMEOUT 20
  LABELS "lint;include-order;essential")

# Test summary and organization
message(STATUS "zpmod test suite configured:")
message(STATUS "  Test categories: core, builtin, command, filesystem, file_io, platform")
message(STATUS "  Total tests: ${CMAKE_CURRENT_LIST_DIR} test files")
message(STATUS "  Enhanced features: timeout control, test categories, improved reporting")

# Note: legacy upstream zsh *.ztst tests present in this folder are ignored by CTest.
