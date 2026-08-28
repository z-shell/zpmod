#!/usr/bin/env zsh

builtin emulate -LR zsh
setopt errexit pipefail

typeset source_root=${0:A:h:h:h}
typeset temp_root
temp_root=$(mktemp -d "${TMPDIR:-/tmp}/zpmod-zi-paths.XXXXXXXX")
trap 'rm -rf -- "$temp_root"' EXIT INT TERM

builtin source "$source_root/scripts/resolve-zi-paths.zsh"

fail_test() {
  print -ru2 -- "$*"
  exit 1
}

assert_equal() {
  [[ $1 == "$2" ]] || fail_test "$3: expected ${(qqq)2}, got ${(qqq)1}"
}

run_case() (
  builtin emulate -LR zsh

  local label=$1 root="$temp_root/$1" expected_modules expected_home expected_layout
  command mkdir -p -- "$root/home"
  typeset -gx HOME="$root/home"
  unset XDG_DATA_HOME
  typeset -gAH ZI
  ZI=()

  case $label in
    active-zi)
      ZI[HOME_DIR]="$root/active home"
      ZI[ZMODULES_DIR]="$root/active modules"
      ZI[COMPLETIONS_DIR]="$root/active completions"
      expected_modules=${ZI[ZMODULES_DIR]}
      expected_home=${ZI[HOME_DIR]}
      expected_layout=active-zi
      ;;
    explicit-home)
      ZI[HOME_DIR]="$root/explicit home"
      expected_home=${ZI[HOME_DIR]}
      expected_modules="$expected_home/zmodules"
      expected_layout=explicit
      ;;
    xdg-missing-spaces)
      typeset -gx XDG_DATA_HOME="$root/data root"
      expected_home="$XDG_DATA_HOME/zi"
      expected_modules="$expected_home/zmodules"
      expected_layout=xdg
      ;;
    xdg-relative)
      typeset -gx XDG_DATA_HOME='relative data'
      expected_home="$HOME/.local/share/zi"
      expected_modules="$expected_home/zmodules"
      expected_layout=xdg
      ;;
    legacy-only)
      command mkdir -p -- "$HOME/.zi/plugins"
      expected_home="$HOME/.zi"
      expected_modules="$expected_home/zmodules"
      expected_layout=legacy
      ;;
    xdg-only)
      typeset -gx XDG_DATA_HOME="$root/data"
      command mkdir -p -- "$XDG_DATA_HOME/zi/plugins"
      expected_home="$XDG_DATA_HOME/zi"
      expected_modules="$expected_home/zmodules"
      expected_layout=xdg
      ;;
    both-external)
      typeset -gx XDG_DATA_HOME="$root/data"
      command mkdir -p -- "$HOME/.zi/plugins" "$XDG_DATA_HOME/zi/plugins"
      expected_home="$HOME/.zi"
      expected_modules="$expected_home/zmodules"
      expected_layout=ambiguous-legacy
      ;;
    both-xdg-source)
      typeset -gx XDG_DATA_HOME="$root/data"
      command mkdir -p -- "$HOME/.zi/plugins" "$XDG_DATA_HOME/zi/plugins"
      ZI[BIN_DIR]="$XDG_DATA_HOME/zi/bin"
      expected_home="$XDG_DATA_HOME/zi"
      expected_modules="$expected_home/zmodules"
      expected_layout=ambiguous-xdg
      ;;
    both-xdg-code)
      typeset -gx XDG_DATA_HOME="$root/data"
      command mkdir -p -- "$HOME/.zi/plugins" "$XDG_DATA_HOME/zi/bin" "$XDG_DATA_HOME/zi/plugins"
      command touch "$XDG_DATA_HOME/zi/bin/zi.zsh"
      expected_home="$XDG_DATA_HOME/zi"
      expected_modules="$expected_home/zmodules"
      expected_layout=ambiguous-xdg
      ;;
  esac

  zpmod_resolve_zi_paths || fail_test "$label resolver failed"
  assert_equal "$REPLY" "$expected_modules" "$label modules"
  assert_equal "$reply[1]" "$expected_home" "$label home"
  assert_equal "$reply[2]" "$expected_layout" "$label layout"
  [[ ! -e $expected_modules ]] || [[ $label == legacy-only || $label == xdg-only || $label == both-* ]] ||
    fail_test "$label created a destination"
)

typeset label
for label in active-zi explicit-home xdg-missing-spaces xdg-relative legacy-only xdg-only both-external both-xdg-source both-xdg-code; do
  run_case "$label"
done

print -r -- 'zi_path_resolution OK'
