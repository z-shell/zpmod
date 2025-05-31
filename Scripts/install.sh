#!/usr/bin/env sh

col_pname="[33m"
col_error="[31m"
col_info="[36m"
col_info2="[32m"
col_rst="[0m"

# Default values
TARGET_DIR=""
CLEAN_BUILD=0
QUIET_MODE=0
VERBOSE_MODE=0
SKIP_GIT=0
FORCE_REBUILD=0
BUILD_ONLY=0
CUSTOM_CFLAGS="-g -Wall -Wextra -O3"
BRANCH="main"
ZSH_EXEC=""
JOBS=""
NO_INSTALL=0
CUSTOM_PREFIX=""

# Output functions that respect verbosity settings
info() {
  if [ "${QUIET_MODE}" -eq 0 ]; then
    printf '%s\n' "$1"
  fi
}

verbose() {
  if [ "${VERBOSE_MODE}" -eq 1 ]; then
    printf '%s\n' "$1"
  fi
}

error() {
  printf '%s\n' "${col_error}$1${col_rst}" >&2
}

show_help() {
  cat <<EOF
Usage: $0 [OPTIONS]

Build and install the zpmod module for zsh

Options:
  --target=DIR, --target DIR   Install to specific directory
  --clean                      Run 'make distclean' instead of 'make clean'
  --quiet, -q                  Suppress non-essential output
  --verbose, -v                Show more detailed build information
  --no-git                     Skip git clone/pull operations
  --force, -f                  Force rebuild even if Makefile exists
  --build-only                 Build but don't update .zshrc
  --cflags="..."               Pass custom CFLAGS to configure (default: -g -Wall -Wextra -O3)
  --branch=NAME                Use specific git branch (default: main)
  --zsh-path=PATH              Use specific Zsh executable
  --jobs=N, -jN                Set number of parallel make jobs
  --prefix=DIR                 Set installation prefix (for system installs)
  --no-install                 Skip installation after building
  --help, -h                   Show this help message
EOF
}

# Check for essential build tools
check_dependencies() {
  for cmd in make gcc; do
    if ! command -v "${cmd}" >/dev/null; then
      error "Required command '${cmd}' not found. Please install it and try again."
      return 1
    fi
  done

  if [ "${SKIP_GIT}" -eq 0 ]; then
    if ! command -v git >/dev/null; then
      error "Git is required but not found. Install git or use --no-git flag."
      return 1
    fi
  fi

  return 0
}

# Parse command line arguments
while [ $# -gt 0 ]; do
  case "$1" in
  --target=*)
    TARGET_DIR="${1#*=}"
    shift
    ;;
  --target)
    if [ -n "$2" ] && [ "${2#-}" = "$2" ]; then
      TARGET_DIR="$2"
      shift 2
    else
      error "Error: --target requires a directory path"
      exit 1
    fi
    ;;
  --clean)
    CLEAN_BUILD=1
    shift
    ;;
  --quiet | -q)
    QUIET_MODE=1
    shift
    ;;
  --verbose | -v)
    VERBOSE_MODE=1
    shift
    ;;
  --no-git)
    SKIP_GIT=1
    shift
    ;;
  --force | -f)
    FORCE_REBUILD=1
    shift
    ;;
  --build-only)
    BUILD_ONLY=1
    shift
    ;;
  --cflags=*)
    CUSTOM_CFLAGS="${1#*=}"
    shift
    ;;
  --branch=*)
    BRANCH="${1#*=}"
    shift
    ;;
  --zsh-path=*)
    ZSH_EXEC="${1#*=}"
    shift
    ;;
  --jobs=* | -j*)
    case "$1" in
    -j*)
      JOBS="${1#-j}"
      ;;
    *)
      JOBS="${1#*=}"
      ;;
    esac
    shift
    ;;
  --prefix=*)
    CUSTOM_PREFIX="${1#*=}"
    # Also set TARGET_DIR based on prefix if not already set
    if [ -z "${TARGET_DIR}" ]; then
      TARGET_DIR="${CUSTOM_PREFIX}/share/zsh/zpmod"
    fi
    shift
    ;;
  --no-install)
    NO_INSTALL=1
    shift
    ;;
  --help | -h)
    show_help
    exit 0
    ;;
  *)
    error "Unknown option: $1"
    info "Use --help to see available options"
    exit 1
    ;;
  esac
done

# Check for dependencies
check_dependencies || exit 1

# Set ZSH executable path
if [ -z "${ZSH_EXEC}" ]; then
  ZSH_EXEC=$(command -v zsh 2>/dev/null)
  if [ -z "${ZSH_EXEC}" ]; then
    error "Zsh is not installed. Please install zsh and try again."
    exit 1
  fi
fi

# Determine ZI_HOME if not provided
if [ -z "${ZI_HOME}" ]; then
  if [ -d "${HOME}/.zi" ]; then
    ZI_HOME="${HOME}/.zi"
  elif [ -d "${ZDOTDIR}/.zi" ]; then
    ZI_HOME="${ZDOTDIR}/.zi"
  elif [ -d "${XDG_DATA_HOME:-${HOME}/.local/share}/.zi" ]; then
    ZI_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}/.zi"
  else
    ZI_HOME="${HOME}/.zi"
  fi
fi

# Determine installation directory - use TARGET_DIR if provided, otherwise default to MOD_HOME
if [ -n "${TARGET_DIR}" ]; then
  info "${col_info}-- Using custom target directory: ${TARGET_DIR} --${col_rst}"
  INSTALL_DIR="${TARGET_DIR}"
else
  if [ -z "${MOD_HOME}" ]; then
    MOD_HOME="${ZI_HOME}/zmodules/zpmod"
  fi
  INSTALL_DIR="${MOD_HOME}"
fi

# Create installation directory if it doesn't exist
if ! test -d "${INSTALL_DIR}"; then
  info "${col_info}-- Creating directory: ${INSTALL_DIR} --${col_rst}"
  mkdir -p "${INSTALL_DIR}"
  chmod g-rwX "${INSTALL_DIR}"
fi

if [ ! -d "${INSTALL_DIR}" ]; then
  error "== Error: Failed to setup module directory =="
  exit 255
fi

# Clone or update repository if not skipped
if [ "${SKIP_GIT}" -eq 0 ]; then
  if test -d "${INSTALL_DIR}/.git"; then
    info "${col_pname}== Updating ZPMOD module at ${INSTALL_DIR} =="
    builtin cd "${INSTALL_DIR}" || exit 255
    command git pull -q origin "${BRANCH}"
  else
    info "${col_pname}== Downloading ZPMOD module to ${INSTALL_DIR} =="
    command git clone --depth 10 -q -b "${BRANCH}" https://github.com/z-shell/zpmod.git "${INSTALL_DIR}"
  fi
  builtin cd "${INSTALL_DIR}" || exit 255
else
  verbose "Skipping git operations as --no-git was specified"
  builtin cd "${INSTALL_DIR}" || exit 255
fi

# Check Zsh version and build the module
info "${col_info2}-- Checking Zsh version --${col_rst}"
ZSH_CURRENT=$("${ZSH_EXEC}" --version </dev/null | head -n1 | cut -d" " -f2,6- | tr -d '-')
ZSH_REQUIRED="5.8.1"

if expr "${ZSH_CURRENT}" \< "${ZSH_REQUIRED}" >/dev/null; then
  error "-- Zsh version 5.8.1 and above required --"
  exit 1
else
  info "${col_info2}-- Zsh version ${ZSH_CURRENT} --${col_rst}"

  # Start build process in a subshell
  (
    info "${col_pname}== Building module ZPMOD =="
    verbose "== The module sources are located at: ${INSTALL_DIR} =="

    # Only clean if Makefile exists and --force is not used
    if test -f Makefile; then
      if [ "${CLEAN_BUILD}" -eq 1 ]; then
        info "${col_info2}-- Running make distclean --${col_rst}"
        command make distclean
      elif [ "${FORCE_REBUILD}" -eq 1 ]; then
        info "${col_info2}-- Forcing rebuild (skipping clean) --${col_rst}"
      else
        info "${col_info2}-- Running make clean --${col_rst}"
        command make clean
      fi
    fi

    # Configure command with custom CFLAGS
    verbose "-- Running configure with CFLAGS: ${CUSTOM_CFLAGS} --"
    ./configure --enable-cflags="${CUSTOM_CFLAGS}" --disable-gdbm --without-tcsetpgrp --quiet || {
      error "Configure failed. See errors above."
      exit 255
    }

    # Determine number of jobs for make
    if [ -z "${JOBS}" ]; then
      cores=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || command getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
    else
      cores="${JOBS}"
    fi

    info "${col_info2}-- Running make with ${cores} jobs --${col_rst}"

    # Capture build output based on verbosity
    if [ "${VERBOSE_MODE}" -eq 1 ]; then
      if ! command make --jobs="${cores}"; then
        error "Module didn't build. See errors above."
        exit 255
      fi
    else
      if ! command make --jobs="${cores}" >make.log 2>&1; then
        error "Module didn't build. See make.log for details."
        exit 255
      fi
    fi

    # Create both .so and .bundle versions
    if [ -f Src/zi/zpmod.so ]; then
      command cp -vf Src/zi/zpmod.so Src/zi/zpmod.bundle

      # Skip installation if requested
      if [ "${NO_INSTALL}" -eq 1 ]; then
        info "${col_info2}-- Module built successfully, skipping installation --${col_rst}"
        return 0
      fi

      # Display success message and instructions
      if [ "${BUILD_ONLY}" -eq 1 ]; then
        info "[38;5;219m[0m [38;5;220mModule [38;5;177mhas been built correctly."
        info "[38;5;219m[0m [38;5;220mFiles are available in ${INSTALL_DIR}/Src"
      else
        command cat <<-EOF
[38;5;219m[0m [38;5;220mModule [38;5;177mhas been built correctly.
[38;5;219m[0m [38;5;220mTo [38;5;160mload the module, add following [38;5;220m2 lines to [38;5;172m.zshrc, at top:

[0m [38;5;51m module_path+=( "${INSTALL_DIR}/Src" )
[0m [38;5;51m zmodload zi/zpmod

[38;5;219m[0m [38;5;220mSee 'zpmod -h' for more information.
[38;5;219m[0m [38;5;220mRun 'zpmod source-study' to see profile data,
[38;5;219m[0m [38;5;177mGuaranteed, automatic compilation of any sourced script.
EOF
      fi
    else
      error "Module didn't build. You can copy the error messages and submit"
      error "error-report at: https://github.com/z-shell/zpmod/issues"
      exit 255
    fi
  )
fi

exit 0
