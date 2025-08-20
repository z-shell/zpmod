#!/usr/bin/env zsh
# Test helper functions for zpmod testing
# Provides enhanced assertion functions while maintaining the current testing approach

# Color output support (force with ZPMOD_TEST_COLOR=1, disable with =0)
_zpmod_init_colors() {
  local use_color=0
  if [[ -n ${ZPMOD_TEST_COLOR:-} ]]; then
    [[ ${ZPMOD_TEST_COLOR} == 1 || ${ZPMOD_TEST_COLOR:l} == always ]] && use_color=1
  elif [[ -t 1 ]]; then
    use_color=1
  fi

  if (( use_color )); then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[0;33m'
    BLUE='\033[0;34m'
    BOLD='\033[1m'
    NC='\033[0m' # No Color
  else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    BOLD=''
    NC=''
  fi
}
_zpmod_init_colors

# Time source helpers (robust under set -u)
_zpmod_now() {
  if (( ${+EPOCHREALTIME} )); then
    print -r -- $EPOCHREALTIME
  elif (( ${+EPOCHSECONDS} )); then
    print -r -- $EPOCHSECONDS
  else
    print -r -- 0
  fi
}

# Test timing support
if [[ -z ${ZPMOD_TEST_START_TIME:-} ]]; then
  ZPMOD_TEST_START_TIME=$(_zpmod_now)
fi

# Location helper: returns "file:line (func)" where assertion was called
_zpmod_loc() {
  local fr line func file
  fr=$(caller 1 2>/dev/null) || fr=$(caller 0 2>/dev/null) || fr="?:? ${0}"
  line=${fr%% *}
  fr=${fr#* }
  if [[ $fr == *" "* ]]; then
    func=${fr%% *}
    file=${fr#* }
  else
    func=""
    file=$fr
  fi
  print -r -- "${file}:${line}${func:+ (${func})}"
}

_zpmod_testname() {
  print -r -- "${TEST_NAME:-${0:t}}"
}

# Pretty printers
_zpmod_print_block() {
  # $1 label, $2 content
  local _label=${1:-Block}
  local _content=${2:-}
  local _lines=0
  if [[ $_content == *$'\n'* ]]; then
    _lines=$(( ${#${(f)_content}} ))
  else
    _lines=1
  fi
  echo -e "  ${BLUE}${_label}${NC} (${_lines} line${_lines==1?"":"s"}):"
  local _line
  while IFS= read -r _line; do
    echo -e "    ${_line}"
  done <<< "${_content}"
}

_zpmod_print_unified_diff() {
  # $1 expected, $2 actual
  local _expected=$1
  local _actual=$2
  if [[ "${_expected}${_actual}" != *$'\n'* ]]; then
    return 1
  fi
  local _e _a _d _rc
  _e=$(mktemp 2>/dev/null) || _e="/tmp/zpmod_expected.$$"
  _a=$(mktemp 2>/dev/null) || _a="/tmp/zpmod_actual.$$"
  print -r -- "$_expected" >| "$_e"
  print -r -- "$_actual"   >| "$_a"
  if command -v diff >/dev/null 2>&1; then
    _d=$(diff -u "$_e" "$_a" 2>/dev/null)
    _rc=$?
    if (( _rc != 0 )) && [[ -n $_d ]]; then
      echo -e "  ${YELLOW}Diff (expected vs actual)${NC}:"
      local _line
      while IFS= read -r _line; do
        case $_line in
          '--- '*|'+++ '*|@@*) echo -e "    ${BLUE}${_line}${NC}" ;;
          -*)                  echo -e "    ${RED}${_line}${NC}"  ;;
          +*)                  echo -e "    ${GREEN}${_line}${NC}";;
          *)                   echo -e "    ${_line}"            ;;
        esac
      done <<< "$_d"
      rm -f -- "$_e" "$_a" 2>/dev/null || true
      return 0
    fi
  fi
  rm -f -- "$_e" "$_a" 2>/dev/null || true
  return 1
}

###
# Enhanced assertion functions
###

# Assert two values are equal
assert_equal() {
  local actual="$1"
  local expected="$2"
  local message="${3:-}"

  if [[ "$actual" != "$expected" ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    local qexp qact
    qexp=$(printf '%q' "$expected")
    qact=$(printf '%q' "$actual")
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Expected${NC}: $qexp"
      echo -e "  ${YELLOW}Actual  ${NC}: $qact"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
      _zpmod_print_unified_diff "$expected" "$actual" || {
        # If no diff printed, still show blocks for multiline values
        if [[ "$expected$actual" == *$'\n'* ]]; then
          _zpmod_print_block "Expected" "$expected"
          _zpmod_print_block "Actual"   "$actual"
        fi
      }
    } >&2
    exit 1
  fi
}

# Assert two values are not equal
assert_not_equal() {
  local actual="$1"
  local expected="$2"
  local message="${3:-}"

  if [[ "$actual" == "$expected" ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    local qval
    qval=$(printf '%q' "$actual")
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Values are equal but should differ${NC}: $qval"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
      if [[ "$actual" == *$'\n'* ]]; then
        _zpmod_print_block "Content" "$actual"
      fi
    } >&2
    exit 1
  fi
}

# Assert string contains substring
assert_contains() {
  local haystack="$1"
  local needle="$2"
  local message="${3:-}"

  if [[ "$haystack" != *"$needle"* ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    local qhay qneed
    qhay=$(printf '%q' "$haystack")
    qneed=$(printf '%q' "$needle")
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Missing substring${NC}: $qneed"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
      if [[ "$haystack$needle" == *$'\n'* ]]; then
        _zpmod_print_block "Haystack" "$haystack"
        _zpmod_print_block "Needle"   "$needle"
      else
        echo -e "  ${YELLOW}In string       ${NC}: $qhay"
      fi
    } >&2
    exit 1
  fi
}

# Assert string does not contain substring
assert_not_contains() {
  local haystack="$1"
  local needle="$2"
  local message="${3:-}"

  if [[ "$haystack" == *"$needle"* ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    local qhay qneed
    qhay=$(printf '%q' "$haystack")
    qneed=$(printf '%q' "$needle")
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Unexpected substring${NC}: $qneed"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
      if [[ "$haystack$needle" == *$'\n'* ]]; then
        _zpmod_print_block "Haystack" "$haystack"
        _zpmod_print_block "Needle"   "$needle"
      else
        echo -e "  ${YELLOW}In string          ${NC}: $qhay"
      fi
    } >&2
    exit 1
  fi
}

# Assert file exists
assert_file_exists() {
  local filepath="$1"
  local message="${2:-}"

  if [[ ! -f "$filepath" ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Missing file${NC}: $filepath"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert file does not exist
assert_file_not_exists() {
  local filepath="$1"
  local message="${2:-}"

  if [[ -f "$filepath" ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Unexpected file exists${NC}: $filepath"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert directory exists
assert_dir_exists() {
  local dirpath="$1"
  local message="${2:-}"

  if [[ ! -d "$dirpath" ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Missing directory${NC}: $dirpath"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert value is empty
assert_empty() {
  local value="$1"
  local message="${2:-}"

  if [[ -n "$value" ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    local qv
    qv=$(printf '%q' "$value")
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Expected empty, got${NC}: $qv"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert value is not empty
assert_not_empty() {
  local value="$1"
  local message="${2:-}"

  if [[ -z "$value" ]]; then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Expected non-empty value${NC}"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert numeric comparison
assert_greater_than() {
  local actual="$1"
  local expected="$2"
  local message="${3:-}"

  if (( actual <= expected )); then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Expected${NC}: actual($actual) > expected($expected)"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert numeric comparison
assert_less_than() {
  local actual="$1"
  local expected="$2"
  local message="${3:-}"

  if (( actual >= expected )); then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Expected${NC}: actual($actual) < expected($expected)"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert array size
assert_array_size() {
  local array_name="$1"
  local expected_size="$2"
  local message="${3:-}"

  # Use parameter expansion to get array size
  local actual_size
  eval "actual_size=\${#${array_name}[@]}"

  if (( actual_size != expected_size )); then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Array size${NC}: got $actual_size, expected $expected_size"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert command succeeds
assert_success() {
  local command="$1"
  local message="${2:-}"

  local out
  out=$(eval "$command" 2>&1)
  local rc=$?
  if (( rc != 0 )); then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Command${NC}: $command"
      echo -e "  ${YELLOW}Exit   ${NC}: $rc"
      [[ -n "$out" ]] && echo -e "  ${YELLOW}Output ${NC}:\n$out"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

# Assert command fails
assert_failure() {
  local command="$1"
  local message="${2:-}"

  local out
  out=$(eval "$command" 2>&1)
  local rc=$?
  if (( rc == 0 )); then
    local loc=$(_zpmod_loc)
    local name=$(_zpmod_testname)
    {
      echo -e "${RED}ASSERTION FAILED${NC} [${BOLD}$name${NC}] at ${BOLD}$loc${NC}"
      echo -e "  ${YELLOW}Command unexpectedly succeeded${NC}: $command"
      [[ -n "$out" ]] && echo -e "  ${YELLOW}Output${NC}:\n$out"
      [[ -n "$message" ]] && echo -e "  ${BLUE}Note${NC}: $message"
    } >&2
    exit 1
  fi
}

###
# Test utility functions
###

# Print test status
test_status() {
  local test_result="$1"
  local test_name="$2"
  local elapsed=""

  if [[ -n ${ZPMOD_TEST_START_TIME:-} ]]; then
    local end_time=$(_zpmod_now)
    local duration=$(( end_time - ZPMOD_TEST_START_TIME ))
    elapsed=$(printf " (%.3fs)" $duration)
  fi

  case "$test_result" in
    "PASS")
      echo -e "${GREEN}✔${NC} ${BOLD}$test_name${NC}$elapsed"
      ;;
    "FAIL")
      echo -e "${RED}✘${NC} ${BOLD}$test_name${NC}$elapsed"
      ;;
    "SKIP")
      echo -e "${YELLOW}●${NC} ${BOLD}$test_name${NC}$elapsed (skipped)"
      ;;
    *)
      echo -e "${BLUE}?${NC} ${BOLD}$test_name${NC}$elapsed ($test_result)"
      ;;
  esac
}

# Skip test with reason
skip_test() {
  local reason="$1"
  echo -e "${YELLOW}SKIPPED${NC}: $reason" >&2
  exit 77  # Standard exit code for skipped tests
}

# Mark test as passed (for explicit success)
pass_test() {
  local message="${1:-Test passed}"
  echo -e "${GREEN}PASSED${NC}: $message"
  exit 0
}

# Print test info
test_info() {
  local message="$1"
  local name=$(_zpmod_testname)
  echo -e "${BLUE}INFO${NC} [${BOLD}$name${NC}]: $message" >&2
}

# Print test debug info (only if ZPMOD_TEST_DEBUG is set)
test_debug() {
  local message="$1"
  if [[ -n ${ZPMOD_TEST_DEBUG:-} ]]; then
  local name=$(_zpmod_testname)
  echo -e "${YELLOW}DEBUG${NC} [${BOLD}$name${NC}]: $message" >&2
  fi
}

###
# Module loading helpers
###

# Load zpmod with error checking
load_zpmod() {
  local features=""
  if (( $# >= 1 )); then
    features=$1
  fi
  local module_dir="${ZPMOD_STAGE_MODULE_DIR:?ZPMOD_STAGE_MODULE_DIR is required}"

  # Ensure zsh can discover the module by name via module_path (robust with set -u)
  test_debug "Prepending to module_path: $module_dir"
  typeset -ga module_path
  # Prepend only if not already present
  if (( ${module_path[(Ie)$module_dir]} == 0 )); then
    module_path=("$module_dir" ${module_path[@]:-})
  fi
  if ! zmodload -i zpmod 2>err.txt; then
    echo -e "${RED}ERROR${NC}: zmodload failed:" >&2
    cat err.txt >&2
    rm -f err.txt
    exit 1
  fi
  rm -f err.txt 2>/dev/null || true

  # Optionally enable feature-gated builtins if provided
  if [[ -n "$features" ]]; then
    local -a _f
    _f=(${(z)features})
    zmodload -e -F zpmod "${_f[@]}" 2>/dev/null || true
  fi

  test_debug "zpmod loaded successfully"
}

# Check if builtin exists
assert_builtin_exists() {
  local builtin_name="$1"
  local message="${2:-Builtin '$builtin_name' should exist}"

  if ! whence -w "$builtin_name" | grep -q "builtin"; then
    echo -e "${RED}ASSERTION FAILED${NC}: $message" >&2
    echo -e "${YELLOW}Available builtins${NC}:" >&2
    whence -w zpmod custom_dot 2>/dev/null || echo "  None found" >&2
    exit 1
  fi
}

# vim:ft=zsh:et:sts=2:sw=2
