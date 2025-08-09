#!/usr/bin/env zsh
# Validate readarray builtin provided by zpmod
set -euo pipefail
emulate -L zsh

: ${ZPMOD_STAGE_MODULE_DIR:?ZPMOD_STAGE_MODULE_DIR is required}
module_path=("$ZPMOD_STAGE_MODULE_DIR" $module_path)

zmodload -i zpmod

# Basic newline-delimited into array A
A=()
print -r -- $'a\nb\nc' | readarray A
(( ${#A[@]} == 3 ))
[[ $A[1] == a && $A[2] == b && $A[3] == c ]]

# Custom delimiter: comma
B=()
print -r -- 'x,y,z' | readarray -d , B
(( ${#B[@]} == 3 ))
[[ $B[1] == x && $B[2] == y && $B[3] == z ]]

# -n count: only first 2 items
C=()
print -r -- $'1\n2\n3' | readarray -n 2 C
(( ${#C[@]} == 2 ))
[[ $C[1] == 1 && $C[2] == 2 ]]

# -O origin: append starting at index 4 (zsh arrays 1-based)
D=(a b c)
print -r -- $'d\ne' | readarray -O 4 D
(( ${#D[@]} == 5 ))
[[ $D[4] == d && $D[5] == e ]]

# -s skip: skip first item
E=()
print -r -- $'skip\nkeep1\nkeep2' | readarray -s 1 E
(( ${#E[@]} == 2 ))
[[ $E[1] == keep1 && $E[2] == keep2 ]]

# -t: strip delimiters; combining with comma delimiter
F=()
print -r -- 'p,q,' | readarray -t -d , F
(( ${#F[@]} == 3 ))
[[ $F[1] == p && $F[2] == q && $F[3] == '' ]]

# -u: read from fd
G=()
{
  exec {fd}<> <(print -r -- $'u1\nu2')
  readarray -u $fd G
  exec {fd}>&-
}
(( ${#G[@]} == 2 ))
[[ $G[1] == u1 && $G[2] == u2 ]]

print -r -- "readarray OK"
