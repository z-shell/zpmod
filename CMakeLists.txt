cmake_minimum_required(VERSION 3.16)
project(zpmod C)

# Enforce a portable C standard across toolchains
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Adopt standard install dir variables and centralize build output directories
include(GNUInstallDirs)
set(ZPMOD_DATA_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}" CACHE INTERNAL
  "Effective architecture-independent data install directory" FORCE)
set(ZPMOD_LICENSE_INSTALL_DIR "${ZPMOD_DATA_INSTALL_DIR}/licenses/zpmod")
set(ZPMOD_DOC_INSTALL_DIR "${ZPMOD_DATA_INSTALL_DIR}/doc/zpmod")

# Export compile_commands.json for clang-tidy/clangd and copy to source dir
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optional performance toggles
option(ZPMOD_ENABLE_LTO "Enable link-time optimization (IPO) for Release builds" ON)
option(ZPMOD_ENABLE_NATIVE "Enable -march=native tuning (non-portable)" OFF)

# Centralized output directories for artifacts produced by the build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)

# Version handling: allow override via -DZPMOD_VERSION, else derive from git
set(ZPMOD_VERSION "" CACHE STRING "zpmod version string override")
if(NOT ZPMOD_VERSION)
  # Attempt to get a descriptive version from git tags
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=7 --dirty --always
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_DESCRIBE
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
  endif()
  if(NOT GIT_DESCRIBE)
    # Fallback to commit hash
    if(GIT_FOUND)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    endif()
  endif()
  if(GIT_DESCRIBE)
    set(ZPMOD_VERSION_RAW "${GIT_DESCRIBE}")
  elseif(GIT_SHA)
    set(ZPMOD_VERSION_RAW "0.0.0+g${GIT_SHA}")
  else()
    set(ZPMOD_VERSION_RAW "0.0.0")
  endif()
  # Strip leading 'v' if present (common tag pattern)
  string(REGEX REPLACE "^v" "" ZPMOD_VERSION "${ZPMOD_VERSION_RAW}")
endif()

# Expose version pieces
set(ZPMOD_GIT_DESCRIBE "${ZPMOD_VERSION}")
string(REGEX MATCH "^[0-9]+" ZPMOD_VERSION_MAJOR "${ZPMOD_VERSION}")
string(REGEX MATCH "^[0-9]+\\.[0-9]+" _minor "${ZPMOD_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" ZPMOD_VERSION_MINOR "${_minor}")
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" _patch "${ZPMOD_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" ZPMOD_VERSION_PATCH "${_patch}")

# Generate version header
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})
file(READ "${CMAKE_SOURCE_DIR}/NOTICE" ZPMOD_NOTICE_TEXT)
file(READ "${CMAKE_SOURCE_DIR}/LICENSES/MIT.txt" ZPMOD_MIT_LICENSE_TEXT)
file(READ "${CMAKE_SOURCE_DIR}/LICENSE" ZPMOD_ZSH_LICENSE_TEXT)
file(WRITE "${GENERATED_DIR}/copyright"
  "${ZPMOD_NOTICE_TEXT}\n\nMIT license text\n----------------\n\n"
  "${ZPMOD_MIT_LICENSE_TEXT}\n\nZsh license text\n----------------\n\n"
  "${ZPMOD_ZSH_LICENSE_TEXT}\n")
configure_file(
  ${CMAKE_SOURCE_DIR}/cmake/zpmod_version.h.in
  ${GENERATED_DIR}/zpmod_version.h
  @ONLY)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Detect zsh headers from submodule first, fallback to local Src
set(ZSH_HEADERS_DIR "${CMAKE_SOURCE_DIR}/vendor/zsh/Src")
if(NOT EXISTS "${ZSH_HEADERS_DIR}/zsh.h")
  # Try to auto-init the submodule if we're in a git checkout
  find_package(Git QUIET)
  if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    message(STATUS "vendor/zsh headers missing; attempting to init submodule (git submodule update --init --recursive -- vendor/zsh)")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- vendor/zsh
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      RESULT_VARIABLE _submodule_rc
      OUTPUT_QUIET
      ERROR_QUIET)
    if(_submodule_rc EQUAL 0 AND EXISTS "${CMAKE_SOURCE_DIR}/vendor/zsh/Src/zsh.h")
      set(ZSH_HEADERS_DIR "${CMAKE_SOURCE_DIR}/vendor/zsh/Src")
      message(STATUS "vendor/zsh submodule initialized successfully")
    endif()
  endif()
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/vendor/zsh/Src/zsh.h")
    message(WARNING "vendor/zsh not initialized; falling back to local Src headers. Run: git submodule update --init --recursive")
    set(ZSH_HEADERS_DIR "${CMAKE_SOURCE_DIR}/Src")
  endif()
endif()

# zsh config.h is expected to be available; prefer vendor/zsh/config.h (from vendored build)
set(ZSH_CONFIG_DIR "${CMAKE_SOURCE_DIR}/vendor/zsh")
set(ZPMOD_HAVE_REAL_CONFIG_H OFF)
if(EXISTS "${ZSH_CONFIG_DIR}/config.h")
  set(ZPMOD_HAVE_REAL_CONFIG_H ON)
else()
  # Next, prefer build/config.h if present (source tree build dir)
  set(ZSH_CONFIG_DIR "${CMAKE_SOURCE_DIR}/build")
  if(EXISTS "${ZSH_CONFIG_DIR}/config.h")
    set(ZPMOD_HAVE_REAL_CONFIG_H ON)
  else()
    # Fallback to repository root if config.h exists there (rare)
    if(EXISTS "${CMAKE_SOURCE_DIR}/config.h")
      set(ZSH_CONFIG_DIR "${CMAKE_SOURCE_DIR}")
      set(ZPMOD_HAVE_REAL_CONFIG_H ON)
    else()
      # As a last resort, generate a minimal placeholder to satisfy include scanners
      set(ZSH_CONFIG_DIR "${CMAKE_BINARY_DIR}")
      file(WRITE "${ZSH_CONFIG_DIR}/config.h" "/* Autogenerated placeholder for zsh config.h; real zsh build not present. */\n")
      message(STATUS "Generated placeholder ${ZSH_CONFIG_DIR}/config.h (HAVE_CONFIG_H will NOT be defined)")
    endif()
  endif()
endif()

add_library(zpmod MODULE
  src/module/module.c
  src/core/fs.c
  src/core/utils.c
  src/core/source.c
  src/core/emoji.c
  src/core/fpath.c
  src/core/source_hot.c
  src/core/source_study_stub.c
  src/core/compaudit_cache.c
  src/core/rehash_diff.c
  src/core/bundle_build.c
  src/builtins/zpmod_builtin.c
  src/builtins/fs_builtins.c
  src/builtins/zpreadarray.c
  src/compat/options.c
)

# Include ordering checker (opt-in target)
add_custom_target(include-order-check
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/check_include_order.zsh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Running include order checker" VERBATIM)

# Define ZPMOD_HAVE_SOURCE_STUDY if source.c is included
if(EXISTS "${ZSH_HEADERS_DIR}/zsh.mdh")
  target_compile_definitions(zpmod PRIVATE ZPMOD_HAVE_SOURCE_STUDY=1)
endif()

# Ensure our module includes zsh headers and config
# Also include src for module-local generated headers (zpmod.mdh/pro if present)

target_include_directories(zpmod PRIVATE
  ${ZSH_CONFIG_DIR}
  ${ZSH_HEADERS_DIR}
  ${CMAKE_SOURCE_DIR}/vendor/zsh/Src
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/module
  ${CMAKE_SOURCE_DIR}/src/include
  ${CMAKE_SOURCE_DIR}/src/compat
  ${GENERATED_DIR}
)

# Define MODULE to match zsh module builds
# Also define necessary feature macros if missing based on config.h

# Define module build and config macros
target_compile_definitions(zpmod PRIVATE MODULE)
if(ZPMOD_HAVE_REAL_CONFIG_H)
  target_compile_definitions(zpmod PRIVATE HAVE_CONFIG_H)
else()
  target_compile_definitions(zpmod PRIVATE ZSH_OOT_MODULE=1)
endif()

# Provide version macros to the build
target_compile_definitions(zpmod PRIVATE
  ZPMOD_VERSION_STR="${ZPMOD_VERSION}"
  ZPMOD_GIT_DESCRIBE_STR="${ZPMOD_GIT_DESCRIBE}")

# Relax symbol resolution; zsh will provide symbols at load time
# No extra link options required; zsh resolves symbols at load time

set_target_properties(zpmod PROPERTIES
  C_STANDARD 99
  PREFIX ""
  OUTPUT_NAME "zpmod"
)

# macOS: allow unresolved symbols to be bound at load time by the host shell
# and keep the conventional .so suffix for zsh modules.
if(APPLE)
  target_link_options(zpmod PRIVATE "-Wl,-undefined,dynamic_lookup")
  set_target_properties(zpmod PROPERTIES SUFFIX ".so")
endif()

# Enable IPO/LTO if supported and requested
if(ZPMOD_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
  if(ipo_ok)
    set_property(TARGET zpmod PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  else()
    message(STATUS "IPO/LTO not supported: ${ipo_msg}")
  endif()
endif()

# Enable -march=native for GCC/Clang if requested (may reduce portability)
if(ZPMOD_ENABLE_NATIVE)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(zpmod PRIVATE -march=native)
  endif()
endif()

# Where to install the zsh module within the install prefix.
# Default to the common third-party location: lib/zsh/site-modules
set(ZPMOD_ZSH_MODDIR "${CMAKE_INSTALL_LIBDIR}/zsh/site-modules" CACHE PATH "Install dir (relative to prefix) for the zpmod shared object")

install(TARGETS zpmod
  LIBRARY DESTINATION ${ZPMOD_ZSH_MODDIR}
  RUNTIME DESTINATION ${ZPMOD_ZSH_MODDIR}
  ARCHIVE DESTINATION ${ZPMOD_ZSH_MODDIR}
)

# Provide a convenient staging target that installs into a local prefix
set(ZPMOD_STAGE_DIR "${CMAKE_BINARY_DIR}/stage" CACHE PATH "Staging prefix for local installs")
add_custom_target(stage
  COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --prefix ${ZPMOD_STAGE_DIR}
  COMMENT "Installing into staging prefix: ${ZPMOD_STAGE_DIR}"
  VERBATIM)

# Basic CPack configuration for packaging (OS-specific generators)
set(CPACK_PACKAGE_NAME "zpmod")
set(CPACK_PACKAGE_VENDOR "z-shell")
set(CPACK_PACKAGE_CONTACT "https://github.com/z-shell/zpmod")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "zpmod: extra builtins and helpers for zsh")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/z-shell/zpmod")
set(CPACK_PACKAGE_VERSION "${ZPMOD_VERSION}")

# Prefer /usr for Linux packages; keep default (/usr/local) on macOS
if(UNIX AND NOT APPLE)
  set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()

# Default generator can be overridden via -DCPACK_GENERATOR or script flag
if(NOT DEFINED CPACK_GENERATOR)
  if(APPLE)
    set(CPACK_GENERATOR "TGZ")
  else()
    set(CPACK_GENERATOR "TGZ")
  endif()
endif()

# Debian package metadata
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "z-shell")
set(CPACK_DEBIAN_PACKAGE_SECTION "shells")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "zsh")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# RPM package metadata
set(CPACK_RPM_PACKAGE_LICENSE "MIT AND LicenseRef-zsh")
set(CPACK_RPM_PACKAGE_GROUP "Shells")
set(CPACK_RPM_PACKAGE_REQUIRES "zsh")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
# RPM packages are Linux artifacts and install below /usr even when CMake is
# configured on another host for metadata validation.
set(ZPMOD_RPM_INSTALL_PREFIX "/usr")
if(IS_ABSOLUTE "${ZPMOD_LICENSE_INSTALL_DIR}")
  set(ZPMOD_RPM_LICENSE_DIR "${ZPMOD_LICENSE_INSTALL_DIR}")
else()
  set(ZPMOD_RPM_LICENSE_DIR
    "${ZPMOD_RPM_INSTALL_PREFIX}/${ZPMOD_LICENSE_INSTALL_DIR}")
endif()
string(REGEX REPLACE "/+" "/" ZPMOD_RPM_LICENSE_DIR
  "${ZPMOD_RPM_LICENSE_DIR}")
set(CPACK_RPM_USER_FILELIST
  "%license ${ZPMOD_RPM_LICENSE_DIR}/LICENSE"
  "%license ${ZPMOD_RPM_LICENSE_DIR}/LicenseRef-zsh.txt"
  "%license ${ZPMOD_RPM_LICENSE_DIR}/MIT.txt"
  "%license ${ZPMOD_RPM_LICENSE_DIR}/NOTICE")

# macOS DMG (DragNDrop) needs no extra metadata

set(CPACK_PACKAGE_FILE_NAME "zpmod-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)

# Convenience: ensure compile_commands.json is available at source root after configure
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(copy-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_BINARY_DIR}/compile_commands.json
      ${CMAKE_SOURCE_DIR}/compile_commands.json
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Sync compile_commands.json to source root for tooling")
endif()

# Locate zsh for runtime smoke testing (prefer vendored build if available)
set(ZSH_EXECUTABLE "" CACHE FILEPATH "Path to zsh to use for smoke/tests")
if(NOT ZSH_EXECUTABLE)
  # Prefer vendored zsh binary if present
  if(EXISTS "${CMAKE_SOURCE_DIR}/vendor/zsh/Src/zsh")
    set(ZSH_EXECUTABLE "${CMAKE_SOURCE_DIR}/vendor/zsh/Src/zsh")
  else()
    find_program(ZSH_EXECUTABLE NAMES zsh)
  endif()
endif()
if(ZSH_EXECUTABLE)
  # Path where the module will be staged
  set(ZPMOD_STAGE_MODULE_DIR "${ZPMOD_STAGE_DIR}/${ZPMOD_ZSH_MODDIR}")
  # Use the same smoke script as CTest to avoid discrepancies in module_path handling
  add_custom_target(smoke
    DEPENDS stage
  COMMAND ${CMAKE_COMMAND} -E env ZPMOD_STAGE_MODULE_DIR=${ZPMOD_STAGE_MODULE_DIR} ${ZSH_EXECUTABLE} -f ${CMAKE_SOURCE_DIR}/tests/core/smoke.zsh
    COMMENT "Running zsh smoke test using staged module and helpers"
    VERBATIM)

  # CTest integration
  include(CTest)
  if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
    # Pass -C <config> only for multi-config generators
    add_custom_target(check
      DEPENDS stage
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure $<$<BOOL:${CMAKE_CONFIGURATION_TYPES}>:-C $<CONFIG>>
      COMMENT "Running CTest suite (depends on 'stage')"
      VERBATIM)
  endif()
else()
  message(WARNING "zsh not found; 'smoke' target will be unavailable")
endif()

# Documentation (Doxygen)
find_package(Doxygen QUIET)
find_package(Graphviz QUIET)
if(DOXYGEN_FOUND)
  set(DOXYFILE_OUT "${CMAKE_BINARY_DIR}/Doxyfile")
  # Provide HAVE_DOT substitution for Doxygen
  if(Graphviz_FOUND OR DOXYGEN_DOT_EXECUTABLE)
    set(HAVE_DOT YES)
  else()
    set(HAVE_DOT NO)
  endif()
  # Pass version into Doxygen as well
  set(ZPMOD_VERSION "${ZPMOD_VERSION}")
  configure_file("${CMAKE_SOURCE_DIR}/cmake/Doxyfile.in" "${DOXYFILE_OUT}" @ONLY)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} "${DOXYFILE_OUT}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
else()
  message(STATUS "Doxygen not found: 'docs' target will be unavailable")
endif()

# Install zsh completion to site-functions
install(FILES ${CMAKE_SOURCE_DIR}/src/completion/_zpmod
        DESTINATION ${CMAKE_INSTALL_DATADIR}/zsh/site-functions
        OPTIONAL)

# Ship every license that applies to the installed binary/completion payload.
install(FILES
        ${CMAKE_SOURCE_DIR}/LICENSE
        ${CMAKE_SOURCE_DIR}/NOTICE
        DESTINATION ${ZPMOD_LICENSE_INSTALL_DIR}
        COMPONENT Licenses)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION ${ZPMOD_LICENSE_INSTALL_DIR}
        RENAME LicenseRef-zsh.txt
        COMPONENT Licenses)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSES/MIT.txt
        DESTINATION ${ZPMOD_LICENSE_INSTALL_DIR}
        RENAME MIT.txt
        COMPONENT Licenses)
install(FILES ${GENERATED_DIR}/copyright
        DESTINATION ${ZPMOD_DOC_INSTALL_DIR}
        RENAME copyright
        COMPONENT Licenses)
