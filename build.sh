#!/usr/bin/env sh

col_pname="[33m"
col_error="[31m"
col_info="[32m"
col_info2="[32m"
col_rst="[0m"

printf '%s\n' "${col_info}Re-run this script to update (from Github) and rebuild the module.$col_rst"

#
# Clone or pull
#

ZI_HOME="${ZI_HOME:-${ZDOTDIR:-${HOME}}/.zi}"
MOD_HOME="${MOD_HOME:-zmodules}/zpmod"

if ! test -d "$ZI_HOME/$MOD_HOME"; then
	mkdir -p "$ZI_HOME/$MOD_HOME"
	chmod g-rwX "$ZI_HOME/$MOD_HOME"
fi

printf '%s\n' ">>> Downloading ZPMOD module to $ZI_HOME/$MOD_HOME"
if test -d "$ZI_HOME/$MOD_HOME/.git"; then
	cd "$ZI_HOME/$MOD_HOME" || return
	git pull origin main
else
	cd "$ZI_HOME" || return
	git clone https://github.com/z-shell/zpmod.git "$MOD_HOME"
fi
printf '%s\n' ">>> Done"

#
# Build the module
#

cd "$ZI_HOME/$MOD_HOME" || return
printf '%s\n' "$col_pname== Building module ZPMOD, running: a make clean, then ./configure and then make ==$col_rst"
printf '%s\n' "$col_pname== The module sources are located at: $ZI_HOME/$MOD_HOME ==$col_rst"
if test -f Makefile; then
	if [ "$1" = "--clean" ]; then
		printf '%s\n' "$col_info2-- make distclean --$col_rst"
		make distclean
		true
	else
		printf '%s\n' "$col_info2-- make clean (pass --clean to invoke \`make distclean') --$col_rst"
		make clean
	fi
fi
printf '%s\n' "$col_info2-- Configuring --$col_rst"
run_make="$(CPPFLAGS=-I/usr/local/include CFLAGS="-g -Wall -O3" LDFLAGS=-L/usr/local/lib ./configure --disable-gdbm --without-tcsetpgrp)"
if [ "$run_make" ]; then
	printf '%s\n' "$col_info2-- make --$col_rst"
	run_make2="$(make)"
	if [ "$run_make2" ]; then
			command cat <<-EOF
			[38;5;219m▓▒░[0m [38;5;220mModule [38;5;177mhas been built correctly.
			[38;5;219m▓▒░[0m [38;5;220mTo [38;5;160mload the module, add following [38;5;220m2 lines to [38;5;172m.zshrc, at top:
			[0m [38;5;51m module_path+=( "$ZI_HOME/$MOD_HOME/Src" )
			[0m [38;5;51m zmodload zi/zpmod
			[38;5;219m▓▒░[0m [38;5;220mSee 'zpmod -h' for more information.
			[38;5;219m▓▒░[0m [38;5;220mRun 'zpmod source-study' to see profile data,
			[38;5;219m▓▒░[0m [38;5;220mGuaranteed, automatic compilation of any sourced script.
		EOF
	else
		printf '%s\n' "${col_error}Module didn't build.$col_rst. You can copy the error messages and submit"
		printf '%s\n' "error-report at: https://github.com/z-shell/zpmod/issues"
	fi
fi