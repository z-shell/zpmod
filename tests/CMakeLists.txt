# Minimal zpmod test suite

if(NOT ZSH_EXECUTABLE)
  return()
endif()

# Compute staged module path
set(ZPMOD_STAGE_MODULE_DIR "${ZPMOD_STAGE_DIR}/${ZPMOD_ZSH_MODDIR}")

# Smoke test: load module from staged directory
add_test(NAME zpmod_smoke
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/smoke.zsh)
set_tests_properties(zpmod_smoke PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# Builtin presence test: verify the 'zpmod' builtin is registered
add_test(NAME zpmod_builtin_present
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/builtin_present.zsh)
set_tests_properties(zpmod_builtin_present PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# custom_dot: verify '.' and 'source' replaced behavior still sources a file
add_test(NAME zpmod_custom_dot
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/custom_dot.zsh)
set_tests_properties(zpmod_custom_dot PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# readarray basic behaviors
add_test(NAME zpmod_readarray
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/readarray.zsh)
set_tests_properties(zpmod_readarray PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# readarray delimiter edge-cases
add_test(NAME zpmod_readarray_delim
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/readarray_delim.zsh)
set_tests_properties(zpmod_readarray_delim PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# readarray: fd + custom delimiter
add_test(NAME zpmod_readarray_fd
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/readarray_fd.zsh)
set_tests_properties(zpmod_readarray_fd PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# readarray: clearing behavior when -O not provided
add_test(NAME zpmod_readarray_clear
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/readarray_clear.zsh)
set_tests_properties(zpmod_readarray_clear PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# report-append memory safety and correctness
add_test(NAME zpmod_report_append
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zpmod_report_append.zsh)
set_tests_properties(zpmod_report_append PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# readarray: large input stress test
add_test(NAME zpmod_readarray_large
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/readarray_large.zsh)
set_tests_properties(zpmod_readarray_large PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# readarray: fd + delimiter + trim
add_test(NAME zpmod_readarray_fd_t
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/readarray_fd_t.zsh)
set_tests_properties(zpmod_readarray_fd_t PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")
# option handling
add_test(NAME zpmod_options
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/options.zsh)
set_tests_properties(zpmod_options PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# New fast FS helpers and reader tests
add_test(NAME zpmod_zpdirlist
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zpdirlist.zsh)
set_tests_properties(zpmod_zpdirlist PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

add_test(NAME zpmod_zppathstat
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zppathstat.zsh)
set_tests_properties(zpmod_zppathstat PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

add_test(NAME zpmod_zpreadfile
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zpreadfile.zsh)
set_tests_properties(zpmod_zpreadfile PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

add_test(NAME zpmod_subcommands
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zpmod_subcommands.zsh)
set_tests_properties(zpmod_subcommands PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

add_test(NAME zpmod_zppathstat_fields
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zppathstat_fields.zsh)
set_tests_properties(zpmod_zppathstat_fields PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

add_test(NAME zpmod_zppathstat_follow
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zppathstat_follow.zsh)
set_tests_properties(zpmod_zppathstat_follow PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

add_test(NAME zpmod_zpdirlist_filters
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zpdirlist_filters.zsh)
set_tests_properties(zpmod_zpdirlist_filters PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

# Additional error/edge tests
add_test(NAME zpmod_zpreadfile_errors
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zpreadfile_errors.zsh)
set_tests_properties(zpmod_zpreadfile_errors PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

add_test(NAME zpmod_zpreadfile_crlf
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zpreadfile_crlf.zsh)
set_tests_properties(zpmod_zpreadfile_crlf PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")

add_test(NAME zpmod_zppathstat_errno_fields
  COMMAND ${ZSH_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/zppathstat_errno_fields.zsh)
set_tests_properties(zpmod_zppathstat_errno_fields PROPERTIES
  ENVIRONMENT "ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR}")


# Note: legacy upstream zsh *.ztst tests present in this folder are ignored by CTest.
