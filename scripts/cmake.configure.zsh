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
  --install-zi               Install zpmod.$ext to Zi modules dir (${ZI[ZMODULES_DIR]}/zpmod)
  --install-user             Install zpmod.$ext to user site-modules (default: ~/.local/lib/zsh/site-modules)
  --install-system           Install system-wide (uses --prefix or defaults to /usr/local)
  --package                  Build a binary package with CPack (default TGZ)
  --cpack-generators <LIST>  Comma-separated CPack generators (e.g., TGZ;TXZ;DEB;RPM)
  --docs                     Build API docs via the CMake 'docs' target (Doxygen)
  --test                     Run a runtime smoke test in zsh after build
  --ctest                    Run the full CTest suite after build/stage
  --ctest-label <LABEL>      Filter CTest by label (repeatable; OR'ed)
  --ctest-regex <REGEX>      Filter CTest by test name regex (-R)
  --ctest-jobs <N>           Parallel CTest jobs (default: same as -j/--jobs)
  --ctest-color              Enable test color output (sets ZPMOD_TEST_COLOR=1)
  --ctest-debug              Enable test debug logs (sets ZPMOD_TEST_DEBUG=1)
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
# CTest controls
DO_CTEST=false
typeset -a CTEST_LABELS
CTEST_LABELS=()
CTEST_REGEX=
CTEST_JOBS=
CTEST_COLOR=false
CTEST_DEBUG=false
# install modes
INSTALL_ZI=false
INSTALL_USER=false
INSTALL_SYSTEM=false

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
  --install-zi)      INSTALL_ZI=true ;;
  --install-user)    INSTALL_USER=true ;;
  --install-system)  INSTALL_SYSTEM=true ;;
  --package)         DO_PACKAGE=true ;;
  --cpack-generators)shift; CPACK_GENERATORS=${1:-} ;;
  --docs)            DO_DOCS=true ;;
    --test)            RUN_TEST=true ;;
  --ctest)           DO_CTEST=true ;;
  --ctest-label)     shift; CTEST_LABELS+=(${1:-}) ;;
  --ctest-regex)     shift; CTEST_REGEX=${1:-} ;;
  --ctest-jobs)      shift; CTEST_JOBS=${1:-} ;;
  --ctest-color)     CTEST_COLOR=true ;;
  --ctest-debug)     CTEST_DEBUG=true ;;
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
_ok "Build complete: $BUILD_DIR/out/lib/zpmod.*"

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

# ---- staged tree hygiene (defensive cleanup) ----
# Some debugging flows may accidentally create a nested 'zsh/zpmod.*' symlink under
# the site-modules directory (e.g., stage/lib/zsh/site-modules/zsh/zpmod.so -> ../zpmod.so).
# This can confuse module discovery. Clean up such stray nested entries if detected.
{
  local moddir="$STAGE_PREFIX/${MOD_SUBDIR:-lib/zsh/site-modules}"
  local nested_dir="$moddir/zsh"
  if [[ -d $nested_dir ]]; then
    local removed=false
    for ext in so bundle dylib dll; do
      local nested="$nested_dir/zpmod.$ext"
      if [[ -L $nested ]]; then
        _warn "Removing stray nested symlink: ${nested:t} in ${nested:h}"
        rm -f -- "$nested" 2>/dev/null && removed=true
      fi
    done
    # Remove the now-empty nested dir (best-effort)
    if $removed; then
      rmdir -- "$nested_dir" 2>/dev/null || true
    fi
  fi
} || true

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

# ---- post-build installs (Zi / user / system) ----
# Determine staged artifact to copy
STAGE_MODDIR=${MOD_SUBDIR:-lib/zsh/site-modules}
STAGED_SO=""
for ext in so bundle dylib dll; do
  test -f "$STAGE_PREFIX/$STAGE_MODDIR/zpmod.$ext" && STAGED_SO="$STAGE_PREFIX/$STAGE_MODDIR/zpmod.$ext" && break
done
if [[ -z $STAGED_SO ]]; then
  for ext in so bundle dylib dll; do
    test -f "$BUILD_DIR/out/lib/zpmod.$ext" && STAGED_SO="$BUILD_DIR/out/lib/zpmod.$ext" && break
  done
fi
[[ -n $STAGED_SO ]] || _warn "Could not locate staged module in $STAGE_PREFIX/$STAGE_MODDIR or $BUILD_DIR/out/lib"

function _copy_so() {
  local src=$1 dst_dir=$2 label=$3
  [[ -f $src ]] || { _die "Source artifact not found: $src"; }
  mkdir -p -- "$dst_dir" || _die "Failed to create $dst_dir"
  local base="${src:t}"  # keep original filename (preserve extension)
  cp -f -- "$src" "$dst_dir/$base" || _die "Failed to copy to $dst_dir"
  _ok "Installed ($label): $dst_dir/$base"
  _print_artifact_hint "$dst_dir/$base"
}

# Print standardized lines with resolved artifact path and module dir
function _print_artifact_hint() {
  local full=$1
  local dir="${full:h}"
  print -r -- "RESOLVED_ARTIFACT=$full"
  print -r -- "RESOLVED_MODULE_DIR=$dir"
  print -r -- "HINT: module_path+=( '$dir' ); zmodload -i zpmod"
}

if $INSTALL_ZI; then
  _msg "Installing for Zi (ZI[ZMODULES_DIR])"
  # Resolve Zi modules root
  zi_modules_root=
  if typeset -p ZI >/dev/null 2>&1 && [[ ${+ZI} -eq 1 && -n ${ZI[ZMODULES_DIR]:-} ]]; then
    zi_modules_root=${ZI[ZMODULES_DIR]}
  elif [[ -n ${XDG_DATA_HOME:-} && -d ${XDG_DATA_HOME} ]]; then
    zi_modules_root="$XDG_DATA_HOME/zi/zmodules"
  else
    zi_modules_root="$HOME/.zi/zmodules"
  fi
  zi_dest="$zi_modules_root/zpmod"
  _copy_so "$STAGED_SO" "$zi_dest" "Zi"
  print -r -- "To load: module_path+=( '$zi_dest' ); zmodload -i zpmod"

  # Optionally symlink completion into Zi's completions directory
  if typeset -p ZI >/dev/null 2>&1 && [[ ${+ZI} -eq 1 && -n ${ZI[COMPLETIONS_DIR]:-} ]]; then
    compdir=${ZI[COMPLETIONS_DIR]}
  src_comp="$REPO_ROOT/src/completion/_zpmod"
    if [[ -f $src_comp ]]; then
      mkdir -p -- "$compdir" || _warn "Cannot create completions dir: $compdir"
      if [[ -e "$compdir/_zpmod" && ! -L "$compdir/_zpmod" ]]; then
        _warn "Completion exists and is not a symlink: $compdir/_zpmod (skipping)"
      else
        ln -sfn -- "$src_comp" "$compdir/_zpmod" && _ok "Linked completion: $compdir/_zpmod" || _warn "Failed to link completion to $compdir"
      fi
    else
      _warn "Completion source not found: $src_comp"
    fi
  else
    _warn "ZI[COMPLETIONS_DIR] not set; skipping completion link"
  fi
fi

if $INSTALL_USER; then
  _msg "Installing for current user (site-modules)"
  # Common user-local path for loadable modules; user must add to module_path
  user_moddir="$HOME/.local/lib/zsh/site-modules"
  _copy_so "$STAGED_SO" "$user_moddir" "user"
  print -r -- "Add to .zshrc: module_path=( '$user_moddir' $module_path ); zmodload -i zpmod"
fi

if $INSTALL_SYSTEM; then
  sys_prefix="${PREFIX:-/usr/local}"
  _msg "Installing system-wide (prefix=$sys_prefix)"
  if cmake --install "$BUILD_DIR" --prefix "$sys_prefix"; then
    _ok "System install complete"
    sys_moddir="$sys_prefix/${MOD_SUBDIR:-lib/zsh/site-modules}"
    print -r -- "If not on MODULE_PATH, add: module_path=( '$sys_moddir' $module_path )"
  else
    _warn "System install failed (permission denied?). Try: sudo cmake --install '$BUILD_DIR' --prefix '$sys_prefix'"
  fi
fi

# ---- runtime smoke test (optional) ----
if $RUN_TEST; then
  _msg "Running CMake 'smoke' target"
  cmake --build "$BUILD_DIR" --target smoke || _die "Smoke test failed"
  _ok "Smoke test passed"
fi

# ---- CTest (optional) ----
if $DO_CTEST; then
  _msg "Running CTest suite"
  # default CTest jobs mirrors build jobs unless overridden
  local cj=${CTEST_JOBS:-$JOBS}
  local -a ctest_cmd
  ctest_cmd=( ctest --test-dir "$BUILD_DIR" --output-on-failure -j "$cj" )
  if (( ${#CTEST_LABELS} )); then
    # OR the labels via a regex like: label1|label2
    local label_regex
    label_regex="${(j:|:)CTEST_LABELS}"
    ctest_cmd+=( -L "$label_regex" )
  fi
  if [[ -n ${CTEST_REGEX:-} ]]; then
    ctest_cmd+=( -R "$CTEST_REGEX" )
  fi
  # environment flags for test helpers
  local -a env_prefix
  env_prefix=()
  $CTEST_COLOR && env_prefix+=( ZPMOD_TEST_COLOR=1 )
  $CTEST_DEBUG && env_prefix+=( ZPMOD_TEST_DEBUG=1 )

  # Use env to safely prefix variables even when negated with '!'
  local -a run_cmd
  if (( ${#env_prefix} )); then
    run_cmd=( env ${=env_prefix} ${=ctest_cmd} )
  else
    run_cmd=( ${=ctest_cmd} )
  fi
  if ! ${=run_cmd}; then
    _die "CTest failed"
  fi
  _ok "CTest passed"
fi

_ok "All done"
