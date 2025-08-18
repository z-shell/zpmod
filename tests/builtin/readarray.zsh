#!/usr/bin/env zsh
# Validate readarray builtin provided by zpmod
set -euo pipefail
emulate -L zsh

# Source test helpers for enhanced reporting
source "${0:A:h:h}/test_helpers.zsh"

# Set test name for reporting
TEST_NAME="builtin/readarray"

# Load module
load_zpmod

# Test description
test_info "Running readarray tests - verifying all readarray functionality"

# Test 1: Basic newline-delimited into array A
test_debug "Testing basic newline-delimited input"
A=()
print -r -- $'a\nb\nc' | readarray A
(( ${#A[@]} == 3 ))
[[ $A[1] == a && $A[2] == b && $A[3] == c ]]

# Test 2: Custom delimiter: comma
test_debug "Testing custom delimiter (comma)"
B=()
print -r -- 'x,y,z' | readarray -d , B
(( ${#B[@]} == 3 ))
[[ $B[1] == x && $B[2] == y && $B[3] == z ]]

# Test 3: -n count: only first 2 items
test_debug "Testing count limitation with -n option"
C=()
print -r -- $'1\n2\n3' | readarray -n 2 C
(( ${#C[@]} == 2 ))
[[ $C[1] == 1 && $C[2] == 2 ]]

# Test 4: -O origin: append starting at index 4 (zsh arrays 1-based)
test_debug "Testing origin offset with -O option"
D=(a b c)
print -r -- $'d\ne' | readarray -O 4 D
(( ${#D[@]} == 5 ))
[[ $D[4] == d && $D[5] == e ]]

# Test 5: -s skip: skip first item
test_debug "Testing skip lines with -s option"
E=()
print -r -- $'skip\nkeep1\nkeep2' | readarray -s 1 E
(( ${#E[@]} == 2 ))
[[ $E[1] == keep1 && $E[2] == keep2 ]]

# Test 6: -t: strip delimiters; combining with comma delimiter
test_debug "Testing strip delimiters with -t option"
F=()
print -r -- 'p,q,' | readarray -t -d , F
(( ${#F[@]} == 3 ))
[[ $F[1] == p && $F[2] == q && $F[3] == '' ]]

# Test 7: -u: read from fd
test_debug "Testing file descriptor input with -u option"
G=()
{
	exec {fd}<> <(print -r -- $'u1\nu2')
	readarray -u $fd G
	exec {fd}>&-
}
(( ${#G[@]} == 2 ))
[[ $G[1] == u1 && $G[2] == u2 ]]

# Test passed
test_status "PASS" "$TEST_NAME"
test_info "readarray functionality test completed successfully"
