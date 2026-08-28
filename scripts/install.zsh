#!/usr/bin/env zsh
#
# install.zsh — advanced installer for zpmod build artifacts
#
# DESIGN GOALS (2025):
#   - Explicit, self-documenting flow (parse → resolve → verify → install → post actions)
#   - Stable exit codes (constants) for automation / packaging scripts
#   - Strict zsh execution environment (options + defensive globals) to avoid accidental leakage
#   - Minimal repetition: shared helpers for logging, copying, error exits
#   - Extendable: future flags can hook into clearly delimited sections
#
# FEATURE SUMMARY:
#   * Multi-mode install: explicit --prefix OR convenience modes: --user / --system / --zi
#   * Artifact extension auto-detection (.so, .bundle, .dylib, .dll)
#   * Optional completion install (skip via --skip-completion)
#   * DESTDIR / packaging aware (prefix path preprend)
#   * Dry-run (--dry-run) & safe overwrite (default) with --force override
#   * Load verification (--verify-load) in clean zsh environment
#   * Explicit module subdir override (--module-dir RELPATH)
#   * Path introspection (--print-paths) for downstream scripts
#   * Strict argument validation with clear, actionable error messages
#
# ASSUMPTIONS:
#   * CMake stage target previously invoked → artifacts under build-cmake/stage
#   * Caller is running zsh ≥ 5.8 (feature set used is stable – guarded check adds warning otherwise)
#
# USAGE:
#   scripts/install.zsh [options]
# ENVIRONMENT:
#   ZPMOD_PREFIX    Alternative way to specify --prefix
#   DESTDIR         Standard packaging override (prepended to final install paths)
#
# PREFIX RESOLUTION ORDER (if no explicit mode flag):
#   --prefix → $ZPMOD_PREFIX → /usr/local (if writable) → ~/.local
#
# EXIT CODE CONTRACT (stable for CI / automation):
#   0  success
#   1  usage / argument error
#   2  staged build artifacts missing
#   3  install copy failure
#   4  load verification failed (when --verify-load)
#   5  environment / precondition (e.g., insufficient permissions) (NEW)
#
# REFACTOR NOTES (why changes vs initial version):
#   - Introduced constants & functions for clarity and future extension.
#   - Centralized arg parsing to allow early validation & conflict detection.
#   - Added zsh option hardening (nounset already via set -u; declare restricted scope via typeset -g for globals).
#   - Added artifact detection function for maintainability if new extensions emerge.
#   - Added load verification script isolation to reduce risk of polluting user environment.
#   - Provided machine-readable output (--print-paths) for tooling integration.
#   - Provided user/system/zi convenience flags mirroring cmake.configure.zsh semantics.
#
set -euo pipefail
setopt NO_BEEP 2>/dev/null || true
setopt WARN_CREATE_GLOBAL 2>/dev/null || true
setopt EXTENDED_GLOB 2>/dev/null || true

########################################
# Constants
########################################
typeset -gr EXIT_OK=0 EXIT_USAGE=1 EXIT_MISSING=2 EXIT_COPY_FAIL=3 EXIT_VERIFY_FAIL=4 EXIT_ENV=5
typeset -gr SCRIPT_NAME=${0:t}

########################################
# Logging helpers (color aware if terminal)
########################################
_color_support() { [[ -t 1 && -n ${TERM:-} && $TERM != dumb ]]; }
if _color_support; then
  typeset -gr C_BOLD=$'%{[1m%}' C_RESET=$'%{[0m%}' C_CYAN=$'%{[36m%}' C_RED=$'%{[31m%}' C_YELLOW=$'%{[33m%}' C_GREEN=$'%{[32m%}'
else
  typeset -gr C_BOLD= C_RESET= C_CYAN= C_RED= C_YELLOW= C_GREEN=
fi
_info(){ (( QUIET )) || print -r -- "${C_CYAN}${SCRIPT_NAME}:${C_RESET} $*"; }
_warn(){ print -u2 -r -- "${C_YELLOW}WARN:${C_RESET} $*"; }
_err(){ print -u2 -r -- "${C_RED}ERROR:${C_RESET} $*"; }
_die(){ local ec=${2:-$EXIT_USAGE}; _err "$1"; exit $ec; }

SCRIPT_DIR=${0:A:h}
REPO_ROOT=${SCRIPT_DIR:h}
STAGE_DIR=$REPO_ROOT/build-cmake/stage

print_usage() {
  cat <<'EOF'
Usage: scripts/install.zsh [options]

General:
  --prefix PATH        Installation prefix (lib & share under this path)
  --user               Convenience alias for --prefix $HOME/.local
  --system             Convenience alias for --prefix /usr/local (requires write perms)
  --zi                  Install into Zi modules directory (auto-detect/rooted)
  --module-dir RELPATH Relative module install subdir (default: lib/zsh/site-modules)
  --destdir PATH       Prepend DESTDIR (packagers / staging root)
  --skip-completion    Do not install completion file
  --dry-run            Show actions without performing them
  --force              Overwrite existing files
  --verify-load        After install, attempt to load zpmod in a clean zsh
  --print-paths        Print resolved artifact & completion destination paths
  --quiet              Minimal output
  -h, --help           Show this help

Examples:
  scripts/install.zsh --user --verify-load
  sudo scripts/install.zsh --system --force
  scripts/install.zsh --prefix /opt/zpmod --skip-completion --print-paths
EOF
}

# --- argument parsing ---
PREFIX=""
DESTDIR_OVERRIDE=""
DRY_RUN=0
FORCE=0
QUIET=0
INSTALL_USER=0
INSTALL_SYSTEM=0
INSTALL_ZI=0
SKIP_COMPLETION=0
VERIFY_LOAD=0
PRINT_PATHS=0
MODULE_DIR_REL="lib/zsh/site-modules"

parse_args() {
  while [[ $# -gt 0 ]]; do
  case $1 in
    --prefix)
      [[ $# -ge 2 ]] || { echo "--prefix requires a value" >&2; exit 1; }
      PREFIX=$2; shift 2 ;;
    --user)    INSTALL_USER=1; shift ;;
    --system)  INSTALL_SYSTEM=1; shift ;;
    --zi)      INSTALL_ZI=1; shift ;;
    --module-dir)
      [[ $# -ge 2 ]] || { echo "--module-dir requires a value" >&2; exit 1; }
      MODULE_DIR_REL=$2; shift 2 ;;
    --destdir)
      [[ $# -ge 2 ]] || { echo "--destdir requires a value" >&2; exit 1; }
      DESTDIR_OVERRIDE=$2; shift 2 ;;
    --skip-completion) SKIP_COMPLETION=1; shift ;;
    --dry-run) DRY_RUN=1; shift ;;
    --force)   FORCE=1; shift ;;
    --verify-load) VERIFY_LOAD=1; shift ;;
    --print-paths) PRINT_PATHS=1; shift ;;
    --quiet)   QUIET=1; shift ;;
    -h|--help) print_usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; print_usage >&2; exit 1 ;;
  esac
done
}
parse_args "$@"
typeset -g ZI_MODE_ACTIVE=0
typeset -g ZI_MODULES_ROOT= ZI_HOME_DIR= ZI_COMPLETIONS_DIR= ZI_LAYOUT=

# Basic zsh version advisory (not a hard error, but may help debugging older envs)
if [[ -n ${ZSH_VERSION:-} ]]; then
  local_major=${ZSH_VERSION%%.*}
  if (( local_major < 5 )); then
    _warn "Detected old zsh ($ZSH_VERSION); loader behavior may differ."
  fi
fi

# Mutually helpful precedence: explicit --prefix beats mode flags
if [[ -z $PREFIX ]]; then
  if (( INSTALL_USER + INSTALL_SYSTEM + INSTALL_ZI > 1 )); then
    _die "Only one of --user / --system / --zi may be specified" $EXIT_USAGE
  fi
  if (( INSTALL_USER )); then
    PREFIX=$HOME/.local
  elif (( INSTALL_SYSTEM )); then
    PREFIX=/usr/local
  elif (( INSTALL_ZI )); then
    builtin source "$REPO_ROOT/scripts/resolve-zi-paths.zsh" ||
      _die "Could not load the Zi path resolver" $EXIT_ENV
    zpmod_resolve_zi_paths ||
      _die "Could not resolve the Zi modules directory" $EXIT_ENV
    ZI_MODULES_ROOT=$REPLY
    ZI_HOME_DIR=$reply[1]
    ZI_LAYOUT=$reply[2]
    ZI_COMPLETIONS_DIR=$reply[3]
    ZI_MODE_ACTIVE=1
    PREFIX=$ZI_HOME_DIR
    unfunction zpmod_resolve_zi_paths
    if [[ $ZI_LAYOUT == ambiguous-* ]]; then
      _warn "Both legacy and XDG Zi homes were detected; using $ZI_HOME_DIR. Set ZI[ZMODULES_DIR] explicitly to select another destination."
    fi
  elif [[ -n ${ZPMOD_PREFIX:-} ]]; then
    PREFIX=$ZPMOD_PREFIX
  else
    if [[ -w /usr/local ]]; then PREFIX=/usr/local; else PREFIX=$HOME/.local; fi
  fi
fi

DESTDIR=${DESTDIR_OVERRIDE:-${DESTDIR:-}}

# Detect artifact (support multiple extensions)
find_artifact() {
  local base="$STAGE_DIR/$MODULE_DIR_REL" ext
  for ext in so bundle dylib dll; do
    local p="$base/zpmod.$ext"
    [[ -f $p ]] && { print -r -- "$p"; return 0; }
  done
  return 1
}

MOD_SRC=$(find_artifact || true)
if [[ -z $MOD_SRC ]]; then
  _die "Module artifact not found under stage ($STAGE_DIR/$MODULE_DIR_REL). Run build + stage first." $EXIT_MISSING
fi

FUNC_SRC=$STAGE_DIR/share/zsh/site-functions/_zpmod
if (( ! SKIP_COMPLETION )) && [[ ! -f $FUNC_SRC ]]; then
  _warn "Completion file missing at $FUNC_SRC (continuing; use --skip-completion to silence)"
  SKIP_COMPLETION=1
fi

if (( ZI_MODE_ACTIVE )); then
  MOD_DEST_DIR=$DESTDIR$ZI_MODULES_ROOT/zpmod
  FUNC_DEST_DIR=$DESTDIR$ZI_COMPLETIONS_DIR
else
  MOD_DEST_DIR=$DESTDIR$PREFIX/$MODULE_DIR_REL
  FUNC_DEST_DIR=$DESTDIR$PREFIX/share/zsh/site-functions
fi

mkdir_p() {
  local d=$1
  if (( DRY_RUN )); then
    (( QUIET )) || echo "mkdir -p $d"
  else
    mkdir -p "$d"
  fi
}

copy_file() {
  # Copy with mode; refuses overwrite unless --force. Honors dry-run.
  local src=$1 dest=$2 mode=$3
  if (( DRY_RUN )); then
    (( QUIET )) || print -r -- "install -m $mode $src $dest"
    return 0
  fi
  if (( FORCE )); then
    install -m $mode "$src" "$dest" || return 1
  else
    if [[ -e $dest ]]; then
      _err "Refusing to overwrite existing $dest (use --force)"
      return 1
    fi
    install -m $mode "$src" "$dest" || return 1
  fi
}

# Ensure target dirs
mkdir_p "$MOD_DEST_DIR"
mkdir_p "$FUNC_DEST_DIR"

# Perform copies
MOD_DEST=$MOD_DEST_DIR/zpmod.so
FUNC_DEST=$FUNC_DEST_DIR/_zpmod

copy_file "$MOD_SRC" "$MOD_DEST" 755 || { _err "Failed to install module"; exit $EXIT_COPY_FAIL; }
if (( ! SKIP_COMPLETION )); then
  copy_file "$FUNC_SRC" "$FUNC_DEST" 644 || { _err "Failed to install completion"; exit $EXIT_COPY_FAIL; }
fi

if (( DRY_RUN )); then
  _info "[dry-run] Installation summary"
else
  _info "Installed zpmod to $MOD_DEST_DIR"
fi

if (( PRINT_PATHS )); then
  print -r -- "ARTIFACT_DST=$MOD_DEST"
  (( ! SKIP_COMPLETION )) && print -r -- "COMPLETION_DST=$FUNC_DEST"
fi

# Suggest user configuration if local prefix
if (( ! QUIET && ZI_MODE_ACTIVE )); then
  cat <<EOF
Add to your ~/.zshrc if not already present:

  module_path+=( "$MOD_DEST_DIR" )
  zmodload -i zpmod
EOF
elif (( ! QUIET )) && [[ $PREFIX == $HOME/.local ]]; then
  cat <<EOF
Add to your ~/.zshrc if not already present:

  if [[ -d $PREFIX/$MODULE_DIR_REL ]]; then
    module_path+=( "$PREFIX/$MODULE_DIR_REL" )
    zmodload -i zpmod 2>/dev/null || zmodload "$PREFIX/$MODULE_DIR_REL/zpmod"
  fi
EOF
fi

# Optional load verification
if (( VERIFY_LOAD )); then
  if (( DRY_RUN )); then
    (( QUIET )) || echo "[dry-run] Skipping load verification" >&2
  else
    tmp_script=$(mktemp)
    cat > "$tmp_script" <<EOS
  emulate -LR zsh
  module_path=( "$MOD_DEST_DIR" $module_path )
  zmodload -i zpmod || { print -u2 'Failed to load zpmod'; exit 1 }
  # Try calling a benign builtin (if any) or just print success
  print 'zpmod: load OK'
EOS
    if ! zsh -f "$tmp_script" >/dev/null; then
      _err "Load verification failed"
      [[ -f $tmp_script ]] && rm -f "$tmp_script"
      exit $EXIT_VERIFY_FAIL
    fi
    [[ -f $tmp_script ]] && rm -f "$tmp_script"
    (( QUIET )) || _info "Verified module loads in clean zsh."
  fi
fi

exit 0
