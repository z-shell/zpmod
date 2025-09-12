#!/usr/bin/env zsh
# SPDX-License-Identifier: MIT
# Simple include order / gateway enforcement script.
# Fails if:
#  - A .c file (under src/) directly includes "zsh.mdh"
#  - System includes (<...>) appear before the gateway pair (zpmod.mdh + zpmod.pro)
#  - Duplicate gateway lines appear back-to-back

set -eu
root_dir=${0:A:h}/..
cd "$root_dir"

typeset -i err=0

for f in $(git ls-files 'src/**/*.c'); do
  # Skip module gateway itself
  [[ $f == src/module/zpmod.mdh ]] && continue

  content=$(< $f)
  if print -- "$content" | grep -q '"zsh.mdh"'; then
    print -u2 "[include-order] $f: direct include of zsh.mdh not allowed (use gateway)"
    err=1
  fi

  have_gateway=0
  duplicate_gateway=0
  line_no=0
  while IFS= read -r line; do
    (( line_no++ )) || true
    # Stop scanning early after first 60 lines (header region)
    (( line_no > 60 )) && break
    [[ $line == \#*include* ]] || continue
    # Normalize whitespace
  norm=${${line##*include }//[[:space:]]/}
  if [[ $norm == "\"zpmod.mdh\"" ]]; then
      if (( have_gateway & 1 )); then duplicate_gateway=1; fi
      (( have_gateway |= 1 ))
      continue
    fi
  if [[ $norm == "\"zpmod.pro\"" ]]; then
      if (( have_gateway & 2 )); then duplicate_gateway=1; fi
      (( have_gateway |= 2 ))
      continue
    fi
    # If system include (#include <...>) appears before both gateway bits set, flag
    if [[ $line =~ '^# *include <' ]] && (( have_gateway != 3 )); then
      print -u2 "[include-order] $f: system include appears before gateway (line $line_no)"
      err=1
      break
    fi
  done < $f
  if (( have_gateway != 3 )); then
    print -u2 "[include-order] $f: missing gateway pair (zpmod.mdh + zpmod.pro) in first 60 lines"; err=1
  fi
  if (( duplicate_gateway )); then
    print -u2 "[include-order] $f: duplicate gateway include lines"; err=1
  fi
done

exit $err
