#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

source "${0:A:h:h}/test_helpers.zsh"
TEST_NAME="filesystem/zppathstat"
load_zpmod
whence -w zppathstat >/dev/null

local -a in=( / /no/such/path )
local -a out
zppathstat out in
(( ${#out} == 2 ))
case ${out[1]} in
	(*type=*) ;;
	(*) print -u2 -- "no type field: ${out[1]}"; exit 1;;
esac
case ${out[2]} in
	(*errno=*) ;;
	(*) print -u2 -- "no errno field: ${out[2]}"; exit 1;;
esac

test_status "PASS" "$TEST_NAME"
exit 0
