#!/usr/bin/env zsh
# Generate and load the TGZ payload rather than testing only the source tree.
emulate -R zsh
setopt err_exit no_unset pipe_fail

typeset source_root=${0:A:h:h:h}
typeset build_dir=${ZPMOD_BUILD_DIR:-$source_root/build-cmake}
typeset scratch
typeset package_dir extract_dir
typeset -a archives package_roots module_files completion_files
typeset -a original_module_path original_fpath
scratch=$(mktemp -d 2>/dev/null || mktemp -d -t zpmod-package-smoke)
trap 'rm -rf -- "$scratch"' EXIT

fail_test() {
  print -ru2 -- "$*"
  exit 1
}

package_dir="$scratch/packages"
extract_dir="$scratch/extracted"
mkdir -p -- "$package_dir" "$extract_dir" "$scratch/home" "$scratch/zdotdir"
export HOME="$scratch/home"
export ZDOTDIR="$scratch/zdotdir"

cpack --config "$build_dir/CPackConfig.cmake" -G TGZ -C Release \
  -B "$package_dir" >/dev/null || fail_test 'TGZ generation failed'
archives=( "$package_dir"/*.tar.gz(N) )
(( ${#archives} == 1 )) || fail_test 'expected exactly one TGZ package'
tar -xzf "$archives[1]" -C "$extract_dir" ||
  fail_test 'TGZ extraction failed'

package_roots=( "$extract_dir"/*(N/) )
(( ${#package_roots} == 1 )) || fail_test 'expected one package root'
module_files=( "$package_roots[1]"/**/lib/zsh/site-modules/zpmod.*(N.) )
completion_files=( "$package_roots[1]"/**/share/zsh/site-functions/_zpmod(N.) )
(( ${#module_files} == 1 )) || fail_test 'package lacks one loadable zpmod module'
(( ${#completion_files} == 1 )) || fail_test 'package lacks the _zpmod completion'

original_module_path=( "${module_path[@]}" )
module_path=( "${module_files[1]:h}" "${original_module_path[@]}" )
zmodload -i zpmod || fail_test 'packaged module failed to load'
[[ $(whence -w zpmod) == *builtin* ]] ||
  fail_test 'packaged module did not register the zpmod builtin'

original_fpath=( "${fpath[@]}" )
fpath=( "${completion_files[1]:h}" "${original_fpath[@]}" )
autoload -Uz _zpmod
[[ $(whence -w _zpmod) == *function* ]] ||
  fail_test 'packaged completion is not autoloadable'

print -r -- 'package_install_smoke OK'
