#!/usr/bin/env sh

#
# Clone or pull
#

setup_zpmod_repository() {
  if ! test -d "${ZI_HOME}/${MOD_HOME}"; then
    mkdir -p "${ZI_HOME}/${MOD_HOME}"
    chmod g-rwX "${ZI_HOME}/${MOD_HOME}"
  fi

  printf '%s\n' "$col_pname== Downloading ZPMOD module to ${ZI_HOME}/${MOD_HOME}"
  if test -d "${ZI_HOME}/${MOD_HOME}/.git"; then
    cd "${ZI_HOME}/${MOD_HOME}" || return
    git pull -q origin main
  else
    cd "$ZI_HOME" || return
    git clone -q https://github.com/z-shell/zpmod.git "$MOD_HOME"
  fi
  printf '%s\n' "$col_pname== Done"
}

#
# Build the module
#

build_zpmod_module() {
  if command -v zsh >/dev/null; then
    printf '%s\n' "$col_info2-- Checkig version --$col_rst"
    ZSH_CURRENT=$(zsh --version </dev/null | head -n1 | cut -d" " -f2,6- | tr -d '-')
    ZSH_REQUIRED="5.8"
    if expr "$ZSH_CURRENT" \< "$ZSH_REQUIRED" >/dev/null; then
      printf '%s\n' "$col_error-- Zsh version 5.8.1 and above required --$col_rst"
      exit 1
    else
      printf '%s\n' "$col_info2-- Zsh version $ZSH_CURRENT --$col_rst"
      cd "${ZI_HOME}/${MOD_HOME}" || return
      printf '%s\n' "$col_pname== Building module ZPMOD, running: a make clean, then ./configure and then make ==$col_rst"
      printf '%s\n' "$col_pname== The module sources are located at: ${ZI_HOME}/${MOD_HOME} ==$col_rst"
      if test -f Makefile; then
        if [ "$1" = "--clean" ]; then
          printf '%s\n' "$col_info2-- make distclean --$col_rst"
          make -s distclean
          true
        else
          printf '%s\n' "$col_info2-- make clean (pass --clean to invoke \`make distclean') --$col_rst"
          make -s clean
        fi
      fi
      printf '%s\n' "$col_info2-- Configuring --$col_rst"
      if CPPFLAGS=-I/usr/local/include CFLAGS="-g -Wall -O3" LDFLAGS=-L/usr/local/lib ./configure --disable-gdbm --without-tcsetpgrp; then
        printf '%s\n' "$col_info2-- Running make --$col_rst"
        if make -s; then
          command cat <<-EOF
[38;5;219m▓▒░[0m [38;5;220mModule [38;5;177mhas been built correctly.
[38;5;219m▓▒░[0m [38;5;220mTo [38;5;160mload the module, add following [38;5;220m2 lines to [38;5;172m.zshrc, at top:

[0m [38;5;51m module_path+=( "$ZI_HOME/$MOD_HOME/Src" )
[0m [38;5;51m zmodload zi/zpmod

[38;5;219m▓▒░[0m [38;5;220mSee 'zpmod -h' for more information.
[38;5;219m▓▒░[0m [38;5;220mRun 'zpmod source-study' to see profile data,
[38;5;219m▓▒░[0m [38;5;177mGuaranteed, automatic compilation of any sourced script.
EOF
        else
          printf '%s\n' "${col_error}Module didn't build.$col_rst. You can copy the error messages and submit"
          printf '%s\n' "error-report at: https://github.com/z-shell/zpmod/issues"
        fi
      fi
    fi
  else
    printf '%s\n' "${col_error} Zsh is not installed. Please install zsh and try again.$col_rst"
  fi
}

MAIN() {

  col_pname="[33m"
  col_error="[31m"
  col_info="[32m"
  col_info2="[32m"
  col_rst="[0m"

  ZI_HOME="${ZI_HOME:-${ZDOTDIR:-${HOME}}/.zi}"
  MOD_HOME="${MOD_HOME:-zmodules}/zpmod"

  if [ ci = 0 ]; then
    printf '%s\n' "${col_info}Re-run this script to update (from Github) and rebuild the module.$col_rst"
    printf '%s\n' "${col_info2}Press any key to continue, or Ctrl-C to exit.$col_rst"
    read -r
  fi    
  setup_zpmod_repository
  build_zpmod_module "$@"
}

while true; do
  MAIN "$@"
  exit 0
done
