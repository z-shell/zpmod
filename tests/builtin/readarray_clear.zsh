#!/usr/bin/env zsh
# verify readarray clears the array when -O is not provided
set -euo pipefail
emulate -L zsh

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="builtin/readarray_clear"
load_zpmod

ARR=(x y z)
print -r -- $'a
b' | readarray ARR
(( ${#ARR[@]} == 2 ))
[[ $ARR[1] == a && $ARR[2] == b ]]

# Ensure no stale elements at higher indices
(( ! ${+ARR[3]} ))

# With -O, previous elements before origin remain, and array is not cleared
ARR=(1 2 3 4)
print -r -- $'e
f' | readarray -O 5 ARR
(( ${#ARR[@]} == 6 ))
[[ $ARR[1] == 1 && $ARR[2] == 2 && $ARR[3] == 3 && $ARR[4] == 4 && $ARR[5] == e && $ARR[6] == f ]]

print -r -- "readarray_clear OK"

test_status "PASS" "$TEST_NAME"
