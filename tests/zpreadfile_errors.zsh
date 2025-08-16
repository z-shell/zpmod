#!/usr/bin/env zsh -f
emulate -L zsh
set -euo pipefail

moddir=${ZPMOD_STAGE_MODULE_DIR:-}
[[ -n $moddir ]] || { print -u2 "no moddir"; exit 99 }
module_path=($moddir $module_path)
zmodload -i zpmod

# ENOENT
local s
if zpreadfile s /path/does/not/exist; then
  print -u2 "zpreadfile succeeded for ENOENT"
  exit 1
fi
[[ -z ${s:-} ]]  # ensure variable not written

# EACCES: create file then chmod 000
local d f
: ${TMPDIR:=/tmp}
d=${TMPDIR%/}/zpmod_rf_err.$RANDOM
mkdir -p $d
f=$d/noaccess
print -r -- "x" > $f
chmod 000 $f || true
if zpreadfile s $f; then
  print -u2 "zpreadfile succeeded for EACCES"
  exit 1
fi
[[ -z ${s:-} ]]

exit 0
