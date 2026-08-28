#!/usr/bin/env zsh
# Resolve Zi-owned zpmod destinations without creating or moving anything.

zpmod_resolve_zi_paths() {
  builtin emulate -L zsh

  local data_base legacy_home xdg_home selected_home layout marker
  integer legacy_present=0 xdg_present=0

  if typeset -p ZI >/dev/null 2>&1 &&
    [[ ${+ZI} -eq 1 && -n ${ZI[ZMODULES_DIR]:-} ]]; then
    REPLY=${ZI[ZMODULES_DIR]}
    selected_home=${ZI[HOME_DIR]:-${REPLY:h}}
    reply=(
      "$selected_home"
      active-zi
      "${ZI[COMPLETIONS_DIR]:-${selected_home}/completions}"
    )
    return 0
  fi

  if [[ -n $XDG_DATA_HOME && $XDG_DATA_HOME == /* ]]; then
    data_base=$XDG_DATA_HOME
  else
    data_base="${HOME}/.local/share"
  fi
  legacy_home="${HOME}/.zi"
  xdg_home="${data_base}/zi"

  if typeset -p ZI >/dev/null 2>&1 &&
    [[ ${+ZI} -eq 1 && -n ${ZI[HOME_DIR]:-} ]]; then
    selected_home=${ZI[HOME_DIR]}
    layout=explicit
  else
    for marker in bin/zi.zsh plugins snippets completions zmodules; do
      if [[ -e "${legacy_home}/${marker}" ]]; then
        legacy_present=1
        break
      fi
    done
    for marker in bin/zi.zsh plugins snippets completions zmodules; do
      if [[ -e "${xdg_home}/${marker}" ]]; then
        xdg_present=1
        break
      fi
    done

    if (( legacy_present && xdg_present )); then
      if { typeset -p ZI >/dev/null 2>&1 &&
        [[ ${+ZI} -eq 1 &&
          ( ${ZI[BIN_DIR]:-} == "${xdg_home}/bin" || ${ZI[BIN_DIR]:-} == "${xdg_home}/bin/"* ) ]] } ||
        { [[ -e "${xdg_home}/bin/zi.zsh" && ! -e "${legacy_home}/bin/zi.zsh" ]] }; then
        selected_home=$xdg_home
        layout=ambiguous-xdg
      else
        selected_home=$legacy_home
        layout=ambiguous-legacy
      fi
    elif (( legacy_present )); then
      selected_home=$legacy_home
      layout=legacy
    else
      selected_home=$xdg_home
      layout=xdg
    fi
  fi

  REPLY="${selected_home}/zmodules"
  reply=( "$selected_home" "$layout" "${selected_home}/completions" )
}
