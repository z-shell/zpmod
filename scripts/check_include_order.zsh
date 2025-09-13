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

  # Disallow direct vendor .epro includes in .c files; use gateway aggregator instead
  if print -- "$content" | grep -E -q '^# *include +"[a-zA-Z0-9_]+\.epro"'; then
    print -u2 "[include-order] $f: direct .epro include not allowed (include zpmod.mdh which aggregates .epro)"
    err=1
  fi

  have_gateway=0
  duplicate_gateway=0
  line_no=0
  first_include=""; second_include=""
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
      if [[ -z $first_include ]]; then first_include="zpmod.mdh";
      elif [[ -z $second_include ]]; then second_include="zpmod.mdh"; fi
      continue
    fi
  if [[ $norm == "\"zpmod.pro\"" ]]; then
      if (( have_gateway & 2 )); then duplicate_gateway=1; fi
      (( have_gateway |= 2 ))
      if [[ -z $first_include ]]; then second_include="zpmod.pro"; first_include=${first_include:-};
      elif [[ -z $second_include ]]; then second_include="zpmod.pro"; fi
      continue
    fi
    # If system include (#include <...>) appears before both gateway bits set, flag
    if [[ $line =~ '^# *include <' ]] && (( have_gateway != 3 )); then
      print -u2 "[include-order] $f: system include appears before gateway (line $line_no)"
      err=1
      break
    fi
    # If any other quoted include appears before gateway pair, flag
    if [[ $line =~ '^# *include "' ]] && (( have_gateway != 3 )); then
      print -u2 "[include-order] $f: project include appears before gateway (line $line_no)"
      err=1
      break
    fi

    # Enforce first-two-include rule: first include must be zpmod.mdh, second must be zpmod.pro
    if [[ -z $first_include ]]; then
      first_include=${norm}
      if [[ $first_include != '"zpmod.mdh"' ]]; then
        print -u2 "[include-order] $f: first include must be \"zpmod.mdh\" (found $first_include)"
        err=1
        break
      fi
      continue
    elif [[ -z $second_include ]]; then
      second_include=${norm}
      if [[ $second_include != '"zpmod.pro"' ]]; then
        print -u2 "[include-order] $f: second include must be \"zpmod.pro\" (found $second_include)"
        err=1
        break
      fi
      continue
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
