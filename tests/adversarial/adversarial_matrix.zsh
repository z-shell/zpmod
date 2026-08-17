#!/usr/bin/env zsh
# Parameterized adversarial-inputs matrix test for zpmod builtins
set -euo pipefail
emulate -L zsh

source "${0:A:h:h}/test_helpers.zsh"

TARGET="${1:-zpdirlist}"
TEST_NAME="adversarial/${TARGET}"
load_zpmod

test_info "Running adversarial input matrix for target: $TARGET"

# Prepare shared scratch fixtures
SCRATCH_DIR=$(mktemp -d "/tmp/zpmod_adv_${TARGET}_XXXXXX")
trap 'rm -rf -- "$SCRATCH_DIR"' EXIT

EMPTY_FILE="$SCRATCH_DIR/empty.txt"
touch "$EMPTY_FILE"

NORMAL_FILE="$SCRATCH_DIR/normal.txt"
print -r -- $'alpha\nbeta\ngamma' > "$NORMAL_FILE"

BINARY_FILE="$SCRATCH_DIR/binary.dat"
printf 'head\x00middle\xFF\xFEtail\x00end' > "$BINARY_FILE"

LARGE_FILE="$SCRATCH_DIR/large.txt"
{
  for (( i=1; i<=10000; i++ )); do
    print -r -- "line_$i data_payload_padding_$i"
  done
} > "$LARGE_FILE"

SAMPLE_DIR="$SCRATCH_DIR/sample_dir"
mkdir -p "$SAMPLE_DIR"
for (( i=1; i<=50; i++ )); do
  touch "$SAMPLE_DIR/file_$i.tmp"
done
mkdir -p "$SAMPLE_DIR/sub_dir"

case "$TARGET" in
  zpdirlist|zpmod_dir_list)
    _run_dirlist() {
      if [[ "$TARGET" == "zpdirlist" ]]; then
        zpdirlist "$@"
      else
        zpmod dir-list "$@"
      fi
    }

    test_debug "Adversarial check: target array entirely unset"
    unset UNSET_OUT || true
    _run_dirlist UNSET_OUT "$SAMPLE_DIR"
    (( ${#UNSET_OUT[@]} > 0 ))

    test_debug "Adversarial check: target array declared but empty"
    typeset -a DECL_OUT
    _run_dirlist DECL_OUT "$SAMPLE_DIR"
    (( ${#DECL_OUT[@]} > 0 ))

    test_debug "Adversarial check: missing directory"
    typeset -a MISSING_OUT
    _run_dirlist MISSING_OUT "$SCRATCH_DIR/nonexistent_dir" 2>/dev/null || true

    test_debug "Adversarial check: regular file passed where directory expected"
    typeset -a FILE_AS_DIR_OUT
    _run_dirlist FILE_AS_DIR_OUT "$NORMAL_FILE" 2>/dev/null || true

    test_debug "Adversarial check: empty directory"
    EMPTY_DIR="$SCRATCH_DIR/empty_dir"
    mkdir -p "$EMPTY_DIR"
    typeset -a EMPTY_DIR_OUT
    _run_dirlist EMPTY_DIR_OUT "$EMPTY_DIR"
    (( ${#EMPTY_DIR_OUT[@]} == 0 ))
    ;;

  zppathstat|zpmod_path_stat)
    _run_pathstat() {
      if [[ "$TARGET" == "zppathstat" ]]; then
        zppathstat "$@"
      else
        zpmod path-stat "$@"
      fi
    }

    test_debug "Adversarial check: target output array entirely unset"
    unset UNSET_OUT || true
    IN_ARR=("$NORMAL_FILE" "$EMPTY_FILE")
    _run_pathstat UNSET_OUT IN_ARR
    (( ${#UNSET_OUT[@]} >= 2 ))

    test_debug "Adversarial check: target output array declared but empty"
    typeset -a DECL_OUT
    _run_pathstat DECL_OUT IN_ARR
    (( ${#DECL_OUT[@]} >= 2 ))

    test_debug "Adversarial check: input array unset"
    unset UNSET_IN || true
    _run_pathstat OUT_RES UNSET_IN 2>/dev/null || true

    test_debug "Adversarial check: input array declared but empty"
    typeset -a EMPTY_IN=()
    typeset -a OUT_EMPTY
    _run_pathstat OUT_EMPTY EMPTY_IN
    (( ${#OUT_EMPTY[@]} == 0 ))

    test_debug "Adversarial check: missing and unreadable file paths in input"
    MIXED_IN=("$SCRATCH_DIR/nonexistent_1" "$NORMAL_FILE" "$SCRATCH_DIR/nonexistent_2")
    typeset -a OUT_MIXED
    _run_pathstat OUT_MIXED MIXED_IN
    (( ${#OUT_MIXED[@]} == 3 ))

    test_debug "Adversarial check: large batch of paths (1,000 files)"
    LARGE_IN=()
    for (( i=1; i<=1000; i++ )); do
      LARGE_IN+=("$SAMPLE_DIR/file_$(( (i % 50) + 1 )).tmp")
    done
    typeset -a OUT_LARGE
    _run_pathstat OUT_LARGE LARGE_IN
    (( ${#OUT_LARGE[@]} == 1000 ))
    ;;

  zpreadfile|zpmod_read_file)
    _run_readfile() {
      if [[ "$TARGET" == "zpreadfile" ]]; then
        zpreadfile "$@"
      else
        zpmod read-file "$@"
      fi
    }

    test_debug "Adversarial check: target variable entirely unset (scalar)"
    unset UNSET_VAR || true
    _run_readfile UNSET_VAR "$NORMAL_FILE"
    assert_contains "$UNSET_VAR" "alpha"

    test_debug "Adversarial check: target variable declared but empty"
    typeset DECL_VAR
    _run_readfile DECL_VAR "$NORMAL_FILE"
    assert_contains "$DECL_VAR" "alpha"

    test_debug "Adversarial check: missing file"
    typeset MISS_VAR
    _run_readfile MISS_VAR "$SCRATCH_DIR/nonexistent_file" 2>/dev/null || true

    test_debug "Adversarial check: directory given where file expected"
    typeset DIR_VAR
    _run_readfile DIR_VAR "$SAMPLE_DIR" 2>/dev/null || true

    test_debug "Adversarial check: empty file"
    typeset EMP_VAR="initial_payload"
    _run_readfile EMP_VAR "$EMPTY_FILE"
    assert_empty "$EMP_VAR"

    test_debug "Adversarial check: binary file with embedded NUL"
    typeset BIN_VAR
    _run_readfile BIN_VAR "$BINARY_FILE"
    (( ${#BIN_VAR} > 0 ))

    test_debug "Adversarial check: large file read (10,000 lines)"
    typeset LARGE_VAR
    _run_readfile LARGE_VAR "$LARGE_FILE"
    assert_contains "$LARGE_VAR" "line_10000"

    test_debug "Adversarial check: array split mode (-m)"
    typeset -a SPLIT_ARR
    _run_readfile -m SPLIT_ARR "$NORMAL_FILE"
    (( ${#SPLIT_ARR[@]} == 3 ))
    [[ $SPLIT_ARR[1] == "alpha" && $SPLIT_ARR[2] == "beta" && $SPLIT_ARR[3] == "gamma" ]]
    ;;

  zpreadarray|readarray)
    _run_readarray() {
      if whence zpreadarray >/dev/null 2>&1; then
        zpreadarray "$@"
      else
        readarray "$@"
      fi
    }

    test_debug "Adversarial check: target array entirely unset"
    unset UNSET_ARR || true
    print -r -- $'line1\nline2\nline3' | _run_readarray UNSET_ARR
    (( ${#UNSET_ARR[@]} == 3 ))

    test_debug "Adversarial check: target array declared but empty"
    typeset -a DECL_ARR
    print -r -- $'item1\nitem2' | _run_readarray DECL_ARR
    (( ${#DECL_ARR[@]} == 2 ))

    test_debug "Adversarial check: empty stream"
    typeset -a EMPTY_ARR
    print -n '' | _run_readarray EMPTY_ARR
    (( ${#EMPTY_ARR[@]} == 0 ))

    test_debug "Adversarial check: non-UTF8 / NUL-separated records"
    typeset -a NUL_RECS
    printf 'part1\x00part2\x00part3' | _run_readarray -d $'\0' NUL_RECS
    (( ${#NUL_RECS[@]} >= 2 ))

    test_debug "Adversarial check: large stream input (10,000 records)"
    typeset -a STREAM_ARR
    _run_readarray STREAM_ARR {fd}< "$LARGE_FILE"
    (( ${#STREAM_ARR[@]} == 10000 ))
    ;;

  zpmod_report_append)
    test_debug "Adversarial check: ZI_REPORTS parameter entirely unset"
    unset ZI_REPORTS || true
    zpmod report-append z-shell/p1 'initial report' 2>/dev/null || true

    test_debug "Adversarial check: ZI_REPORTS declared but empty (Issue #80 guard)"
    typeset -A ZI_REPORTS
    zpmod report-append z-shell/p1 'second report' 2>/dev/null || true

    test_debug "Adversarial check: ZI_REPORTS initialized with empty target key"
    ZI_REPORTS=()
    ZI_REPORTS['z-shell/p1']=''
    zpmod report-append z-shell/p1 '+appended_chunk'
    [[ ${ZI_REPORTS['z-shell/p1']} == '+appended_chunk' ]]

    test_debug "Adversarial check: append large payload (100KB)"
    LARGE_PAYLOAD=$(head -c 102400 < /dev/zero | tr '\0' 'x')
    zpmod report-append z-shell/p1 "$LARGE_PAYLOAD"
    (( ${#ZI_REPORTS['z-shell/p1']} > 100000 ))

    test_debug "Adversarial check: missing arguments"
    zpmod report-append 2>/dev/null || true
    zpmod report-append z-shell/p1 2>/dev/null || true
    ;;

  *)
    echo "Unknown adversarial target: $TARGET" >&2
    exit 1
    ;;
esac

test_status "PASS" "$TEST_NAME"
test_info "Adversarial input matrix passed for $TARGET"
