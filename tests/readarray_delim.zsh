#!/usr/bin/env zsh
# Extra delimiter edge cases for readarray
set -euo pipefail
emulate -L zsh

: ${ZPMOD_STAGE_MODULE_DIR:?ZPMOD_STAGE_MODULE_DIR is required}
module_path=("$ZPMOD_STAGE_MODULE_DIR" $module_path)

zmodload -i zpmod

# Trailing record without delimiter
X=()
print -nr -- 'a,b' | readarray -d , X
(( ${#X[@]} == 2 ))
[[ $X[1] == a && $X[2] == b ]]

# Keep delimiter (-t off)
Y=()
print -nr -- 'p,q,' | readarray -d , Y
(( ${#Y[@]} == 3 ))
[[ $Y[1] == 'p,' && $Y[2] == 'q,' && $Y[3] == '' ]]

# Remove delimiter (-t)
Z=()
print -nr -- 'm,n,' | readarray -t -d , Z
(( ${#Z[@]} == 3 ))
[[ $Z[1] == m && $Z[2] == n && $Z[3] == '' ]]

# Skip with custom delimiter
S=()
print -nr -- 's1;s2;s3' | readarray -s 1 -d ';' S
(( ${#S[@]} == 2 ))
[[ $S[1] == s2 && $S[2] == s3 ]]

print -r -- "readarray_delim OK"
