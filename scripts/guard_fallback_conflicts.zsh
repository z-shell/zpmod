#!/usr/bin/env zsh
# Guard script: detect macro vs extern collisions between fallback header and vendored zsh headers.
# Fails (exit 1) if any symbol declared as extern variable in `zpmod_internals.h` is also a simple
# object-like macro (no params) in vendor/zsh. Intentionally lightweight (grep / sed based) to avoid
# adding build deps.

set -eu

repo_root=${0:A:h:h}
fallback=$repo_root/src/include/zpmod_internals.h
vendor_dir=$repo_root/vendor/zsh

if [[ ! -f $fallback ]]; then
  print -u2 "Fallback header not found: $fallback"; exit 2
fi
if [[ ! -d $vendor_dir ]]; then
  print -u2 "Vendor zsh dir not found: $vendor_dir"; exit 2
fi

# Collect extern variable names (exclude function prototypes by filtering out '(')
extern_syms=(${(f)$(grep -E '^extern ' $fallback | grep -v '(' | \
  sed -E 's/^extern[[:space:]]+([^;]+);/\1/' | \
  awk '{print $NF}' | sed 's/;//' | sort -u)})

# Build associative array of macro -> file for vendor object-like macros
typeset -A macro_map
while IFS= read -r line; do
  # line format: FILE: #define NAME value
  file=${line%%:*}
  rest=${line#*:}
  name=$(print -- $rest | awk '{print $2}')
  [[ -n $name ]] && macro_map[$name]=$file
done < <(grep -R --include='*.h' -n '^#define[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]+\([^()]' $vendor_dir 2>/dev/null || true)

conflicts=()
for s in $extern_syms; do
  if [[ -n ${macro_map[$s]:-} ]]; then
    conflicts+=$s
  fi
done

if (( ${#conflicts} )); then
  print -u2 "Conflict: fallback declares extern(s) also defined as macro(s) upstream: ${conflicts[*]}"
  for c in $conflicts; do
    print -u2 "  $c -> ${macro_map[$c]}"
  done
  exit 1
fi

print -- "No macro/extern collisions detected (${#extern_syms} extern symbols scanned)."
