#!/usr/bin/env zsh

emulate -LR zsh
setopt err_exit no_unset pipe_fail

typeset -r repo_root=${0:A:h:h}
typeset temp_dir
temp_dir=$(mktemp -d "${TMPDIR:-/tmp}/zpmod-system-install.XXXXXX")
trap 'rm -rf -- "$temp_dir"' EXIT

typeset -r source_dir=$temp_dir/source
typeset -r stage_dir=$temp_dir/stage
typeset -r shim_dir=$temp_dir/shims
mkdir -p "$source_dir" "$stage_dir" "$shim_dir"

git -C "$repo_root" archive HEAD | tar -x -C "$source_dir"

for command_name in git curl; do
  cat > "$shim_dir/$command_name" <<EOF
#!/bin/sh
echo "unexpected $command_name invocation during package build" >&2
exit 97
EOF
  chmod +x "$shim_dir/$command_name"
done

typeset -r build_path=$shim_dir:$PATH
typeset -r module_dir=$(zsh -fc 'print -r -- $module_path[1]')

(
  cd "$source_dir"
  PATH=$build_path ./configure \
    --prefix=/usr \
    --enable-cflags="-g -Wall -Wextra -O3" \
    --disable-gdbm \
    --without-tcsetpgrp \
    --quiet
  PATH=$build_path make --jobs=2
  PATH=$build_path make \
    DESTDIR="$stage_dir" \
    MODDIR="$module_dir" \
    install
)

typeset -r dl_ext=$(
  sed -n 's/^DL_EXT[[:space:]]*=[[:space:]]*//p' \
    "$source_dir/Config/defs.mk"
)
typeset -r installed_module=$stage_dir$module_dir/zi/zpmod.$dl_ext
typeset -a installed_files
installed_files=( "$stage_dir"/**/*(.N) )

if (( ${#installed_files} != 1 )) ||
   [[ ${installed_files[1]:-} != $installed_module ]]; then
  print -u2 "Unexpected staged files:"
  print -u2 -l -- "${installed_files[@]}"
  exit 1
fi

zsh "$source_dir/Test/ci-module.zsh" "$stage_dir$module_dir"

(
  cd "$source_dir"
  PATH=$build_path make \
    DESTDIR="$stage_dir" \
    MODDIR="$module_dir" \
    uninstall
)

if [[ -e $installed_module ]]; then
  print -u2 "Uninstall left the module in place: $installed_module"
  exit 1
fi
