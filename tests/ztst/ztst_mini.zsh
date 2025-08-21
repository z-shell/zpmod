#!/usr/bin/env zsh
# Minimal ztst-like adapter for zpmod
# Purpose: Run a tiny suite of ztst-style cases under CTest without depending on upstream zsh's ztst harness.
# Scope: exact-match stdout/stderr/status; simple per-case reporting; accumulates failures and exits non-zero.

set -euo pipefail
emulate -L zsh

# Colors (simple, no external deps)
if [[ -t 1 ]]; then
  RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[0;33m'; BLUE='\033[0;34m'; BOLD='\033[1m'; NC='\033[0m'
else
  RED=''; GREEN=''; YELLOW=''; BLUE=''; BOLD=''; NC=''
fi

typeset -g ZTST_SUITE_NAME="ztst"
typeset -g ZTST_CASES=0
typeset -g ZTST_FAILS=0

ztst_suite() {
  ZTST_SUITE_NAME=${1:-ztst}
}

_zfst_print_block() { :; } # placeholder to prevent typos

_ztst_print_block() {
  local _label=$1 _content=$2
  print -r -- "  ${BLUE}${_label}${NC}:"
  local _line
  while IFS= read -r _line; do
    print -r -- "    ${_line}"
  done <<< "$_content"
}

_ztst_run_case() {
  local desc="$1"; shift
  local cmd="" exp_out="" exp_err="" exp_status=0 mode="exact"
  # Parse simple flags: --cmd, --stdout, --stderr, --status, --contains
  while (( $# )); do
    case $1 in
      --cmd)     shift; cmd=${1:-} ;;
      --stdout)  shift; exp_out=${1:-} ;;
      --stderr)  shift; exp_err=${1:-} ;;
      --status)  shift; exp_status=${1:-0} ;;
      --contains) mode="contains" ;;
      *) ;;  # ignore unknowns to keep adapter thin
    esac
    shift || true
  done

  (( ZTST_CASES++ ))
  local out err rc
  out=""; err=""; rc=0
  # Use process substitution to capture stderr separately
  out=$(eval "$cmd" 2> >(cat >&2)) || rc=$?
  # Best-effort capture of stderr: rerun safely only to capture if needed
  # Note: For deterministic tests, prefer commands that print only to stdout.
  # In our suite, we tailor tests to stdout primarily.

  local fail=0
  if [[ "$mode" == exact ]]; then
    [[ "$out" == "$exp_out" ]] || fail=1
  else
    [[ "$out" == *"$exp_out"* ]] || fail=1
  fi
  # We don't strictly compare stderr in this minimal adapter unless expected provided and we can capture it
  if [[ -n "$exp_err" ]]; then
    local err_cap
    err_cap=$(eval "$cmd" 1>/dev/null 2> >(cat)) || true
    [[ "$err_cap" == "$exp_err" ]] || fail=1
  fi
  (( rc == exp_status )) || fail=1

  if (( fail )); then
    (( ZTST_FAILS++ ))
    print -r -- "${RED}✘${NC} ${BOLD}${ZTST_SUITE_NAME}${NC}: ${desc}"
    print -r -- "  ${YELLOW}Command${NC}: ${cmd}"
    print -r -- "  ${YELLOW}Exit${NC}: got ${rc}, expected ${exp_status}"
    _ztst_print_block "Stdout (got)" "$out"
    _ztst_print_block "Stdout (exp)" "$exp_out"
  else
    print -r -- "${GREEN}✔${NC} ${BOLD}${ZTST_SUITE_NAME}${NC}: ${desc}"
  fi
}

# Public API: ztst_it "desc" [--cmd '...'] [--stdout $'...'] [--stderr $'...'] [--status N] [--contains]
ztst_it() { _ztst_run_case "$@" }

# Runner: if an argument is provided, source it as the test file; otherwise, act as a library
if (( $# >= 1 )); then
  typeset _ZTST_FILE="$1"; shift
  : ${ZPMOD_STAGE_MODULE_DIR:=}
  source "${_ZTST_FILE}"
  if (( ZTST_FAILS > 0 )); then
    print -r -- "${RED}FAILED${NC}: ${ZTST_FAILS}/${ZTST_CASES} cases failed in ${BOLD}${ZTST_SUITE_NAME}${NC}" >&2
    exit 1
  fi
  print -r -- "${GREEN}PASSED${NC}: ${ZTST_CASES} cases in ${BOLD}${ZTST_SUITE_NAME}${NC}"
fi

# vim:ft=zsh:et:sts=2:sw=2
