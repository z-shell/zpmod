#!/usr/bin/env zsh
#
# cmake.configure.zsh — configure, build, and optionally test zpmod with CMake
#
# This script will:
#  - ensure vendor/zsh submodule exists (and init it if missing)
#  - build vendored zsh to generate config.h and generated headers (.mdh/.epro)
#  - configure and build zpmod with CMake
#  - optionally stage, package, install, and run a runtime smoke test
#
# Requirements: zsh, git, cmake, make (or ninja if selected), a C compiler

set -e
set -u
set -o pipefail

# ---- helpers ----
autoload -Uz colors || true
colors || true

: ${TERM:=dumb}

function _msg() { print -r -- "$fg_bold[cyan]::${1:+ $1}$reset_color"; }
function _ok()  { print -r -- "$fg_bold[green]✔$reset_color ${1:-done}"; }
function _warn(){ print -r -- "$fg_bold[yellow]⚠$reset_color ${1:-warning}"; }
function _err() { print -u2 -r -- "$fg_bold[red]✖$reset_color ${1:-error}"; }
function _die() { _err "$1"; exit ${2:-1}; }

function usage() {
  cat <<'USAGE'
Usage: scripts/cmake.configure.zsh [options]

Options:
  --init-submodule           Force init/update vendor/zsh submodule
  --no-submodule             Skip submodule init (assume present)
  --vendor-build             Force building vendor/zsh (./configure && make)
  --no-vendor-build          Skip vendor build (assume headers ready)
  --generator <make|ninja>   Choose CMake generator (default: make)
  --build-type <TYPE>        CMAKE_BUILD_TYPE (default: Release)
  -j, --jobs <N>             Parallel jobs for make/ninja (default: auto)
  --reconfigure              Re-run CMake configure from scratch
  --clean                    Remove build-cmake/ before configuring
  --prefix <DIR>             Install prefix for 'cmake --install'
  --stage-prefix <DIR>       Staging prefix for local install (default: build-cmake/stage)
  --moddir <REL-PATH>        Install subdir for module (relative to prefix). Default: lib/zsh/site-modules
  --package                  Build a binary package with CPack (default TGZ)
  --cpack-generators <LIST>  Comma-separated CPack generators (e.g., TGZ;TXZ;DEB;RPM)
  --docs                     Build API docs via the CMake 'docs' target (Doxygen)
  --test                     Run a runtime smoke test in zsh after build
  --verbose                  Verbose CMake and make/ninja output
  -h, --help                 Show this help and exit

Behavior:
- If vendor/zsh/config.h is missing and vendor build isn't disabled, the script
  builds vendor/zsh to generate headers/prototypes expected by zpmod.
- CMake build directory: ./build-cmake
USAGE
}

# ---- resolve paths ----
SCRIPT_DIR=${0:A:h}
REPO_ROOT=${SCRIPT_DIR:h}
cd "$REPO_ROOT"

# ---- defaults ----
DO_SUBMODULE=auto
DO_VENDOR_BUILD=auto
GENERATOR=make
BUILD_TYPE=Release
JOBS=
RECONFIGURE=false
CLEAN=false
PREFIX=
STAGE_PREFIX=
MOD_SUBDIR=
DO_PACKAGE=false
CPACK_GENERATORS=
DO_DOCS=false
RUN_TEST=false
VERBOSE=false

# ---- arg parsing ----
args=()
while (( $# > 0 )); do
  case "$1" in
    --init-submodule)  DO_SUBMODULE=true ;;
    --no-submodule)    DO_SUBMODULE=false ;;
    --vendor-build)    DO_VENDOR_BUILD=true ;;
    --no-vendor-build) DO_VENDOR_BUILD=false ;;
    --generator)       shift; GENERATOR=${1:-make} ;;
    --build-type)      shift; BUILD_TYPE=${1:-Release} ;;
    -j|--jobs)         shift; JOBS=${1:-} ;;
    --reconfigure)     RECONFIGURE=true ;;
    --clean)           CLEAN=true ;;
    --prefix)          shift; PREFIX=${1:-} ;;
  --stage-prefix)    shift; STAGE_PREFIX=${1:-} ;;
  --moddir)          shift; MOD_SUBDIR=${1:-} ;;
  --package)         DO_PACKAGE=true ;;
  --cpack-generators)shift; CPACK_GENERATORS=${1:-} ;;
  --docs)            DO_DOCS=true ;;
    --test)            RUN_TEST=true ;;
    --verbose)         VERBOSE=true ;;
    -h|--help)         usage; exit 0 ;;
    *)                 args+=$1 ;;
  esac
  shift
done

# unused positional args guard
if (( ${#args} )); then
  _warn "Ignoring extra arguments: ${args[*]}"
fi

# ---- prereq checks ----
for cmd in git cmake zsh; do
  command -v $cmd >/dev/null 2>&1 || _die "$cmd not found in PATH"
done
if [[ $GENERATOR == ninja ]]; then
  command -v ninja >/dev/null 2>&1 || _die "ninja not found; use --generator make or install ninja"
else
  command -v make  >/dev/null 2>&1 || _die "make not found; install build tools"
fi

# jobs autodetect
if [[ -z ${JOBS:-} ]]; then
  if command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc)
  elif command -v sysctl >/dev/null 2>&1; then
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)
  else
    JOBS=2
  fi
fi

# ---- submodule init ----
if [[ $DO_SUBMODULE == auto ]]; then
  if [[ ! -d vendor/zsh/.git ]]; then
    DO_SUBMODULE=true
  else
    DO_SUBMODULE=false
  fi
fi
if [[ $DO_SUBMODULE == true ]]; then
  _msg "Initializing submodule vendor/zsh"
  git submodule update --init --recursive vendor/zsh || _die "Failed to init submodule"
  _ok "vendor/zsh initialized"
fi

# ---- vendor build (zsh) ----
need_vendor=false
if [[ $DO_VENDOR_BUILD == auto ]]; then
  [[ -f vendor/zsh/config.h ]] || need_vendor=true
  DO_VENDOR_BUILD=$need_vendor
fi
if [[ $DO_VENDOR_BUILD == true ]]; then
  _msg "Building vendor/zsh to generate headers"
  (
    set -e
    cd vendor/zsh
    if [[ ! -x ./configure ]]; then
      if [[ -x ./Util/preconfig ]]; then
        ./Util/preconfig
      fi
    fi
    if [[ ! -f config.h ]]; then
      ./configure || _die "vendor/zsh configure failed"
    fi
    make -j $JOBS || _die "vendor/zsh make failed"
    [[ -f config.h ]] || _die "vendor/zsh/config.h not generated"
    [[ -f Src/zsh.mdh ]] || _warn "vendor/zsh/Src/zsh.mdh missing (will be generated during build)."
  )
  _ok "vendor/zsh built"
else
  _msg "Skipping vendor build"
fi

# ---- CMake configure ----
BUILD_DIR=$REPO_ROOT/build-cmake
if $CLEAN; then
  _msg "Cleaning $BUILD_DIR"
  rm -rf -- "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

cmake_args=( -S "$REPO_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" )
if [[ $GENERATOR == ninja ]]; then
  cmake_args=( -G Ninja $cmake_args )
fi
if $VERBOSE; then
  cmake_args+=( -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON )
fi
if [[ -n ${MOD_SUBDIR:-} ]]; then
  cmake_args+=( -DZPMOD_ZSH_MODDIR="$MOD_SUBDIR" )
fi
if [[ -n ${CPACK_GENERATORS:-} ]]; then
  cmake_args+=( -DCPACK_GENERATOR="$CPACK_GENERATORS" )
fi
# Prefer vendored zsh for tests if present
if [[ -x vendor/zsh/Src/zsh ]]; then
  cmake_args+=( -DZSH_EXECUTABLE="$PWD/vendor/zsh/Src/zsh" )
fi
if $RECONFIGURE && [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  _msg "Reconfiguring: removing existing CMakeCache.txt"
  rm -f "$BUILD_DIR/CMakeCache.txt"
fi

_msg "Configuring CMake ($GENERATOR, $BUILD_TYPE)"
cmake ${cmake_args[@]} || _die "CMake configure failed"
_ok "CMake configured"

# ---- build ----
_msg "Building (jobs: $JOBS)"
cmake --build "$BUILD_DIR" -j "$JOBS" ${VERBOSE:+-v} || _die "Build failed"
_ok "Build complete: $BUILD_DIR/zpmod.so"

# ---- docs (optional) ----
if $DO_DOCS; then
  _msg "Building docs (CMake target 'docs')"
  if cmake --build "$BUILD_DIR" --target docs; then
    if [[ -d "$BUILD_DIR/docs/html" ]]; then
      _ok "Docs generated at: $BUILD_DIR/docs/html"
    else
      _warn "Docs target ran but output not found at $BUILD_DIR/docs/html"
    fi
  else
    _warn "Docs build failed (Doxygen not installed?)"
  fi
fi

# ---- staging (optional) ----
if [[ -z ${STAGE_PREFIX:-} ]]; then
  STAGE_PREFIX="$BUILD_DIR/stage"
fi
_msg "Staging into $STAGE_PREFIX"
cmake --install "$BUILD_DIR" --prefix "$STAGE_PREFIX" || _die "Stage install failed"
_ok "Staged to $STAGE_PREFIX"

# ---- packaging (optional) ----
if $DO_PACKAGE; then
  _msg "Packaging with CPack (${CPACK_GENERATORS:-TGZ})"
  (
    cd "$BUILD_DIR"
    if [[ -n ${CPACK_GENERATORS:-} ]]; then
      cpack -G "$CPACK_GENERATORS" -C "$BUILD_TYPE"
    else
      cpack -C "$BUILD_TYPE"
    fi
  ) || _die "CPack packaging failed"
  _ok "Packages created in $BUILD_DIR"
fi

# ---- install (optional) ----
if [[ -n ${PREFIX:-} ]]; then
  _msg "Installing to $PREFIX"
  cmake --install "$BUILD_DIR" --prefix "$PREFIX" || _die "Install failed"
  _ok "Installed"
fi

# ---- runtime smoke test (optional) ----
if $RUN_TEST; then
  _msg "Running CMake 'smoke' target"
  cmake --build "$BUILD_DIR" --target smoke || _die "Smoke test failed"
  _ok "Smoke test passed"
fi

_ok "All done"
