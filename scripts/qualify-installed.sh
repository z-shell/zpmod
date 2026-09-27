#!/usr/bin/env bash
# CI integration driver, Bash 3.2+. Runs after the full CTest suite.
set -euo pipefail
: "${ZSH_EXECUTABLE:?}" "${RUNNER_TEMP:?}"
root=$(pwd)
scratch=$(mktemp -d "${RUNNER_TEMP}/zpmod-installed.XXXXXX")
# Keep packages and evidence until the runner ends; no system install.
cmake --install build-cmake --prefix "${scratch}/prefix"
cpack --config build-cmake/CPackConfig.cmake -G TGZ -C Release -B "${scratch}/packages"
mkdir -p "${scratch}/extracted" qualification-results
archives=("${scratch}"/packages/*.tar.gz)
[[ ${#archives[@]} == 1 && -f ${archives[0]} ]]
tar -xzf "${archives[0]}" -C "${scratch}/extracted"
package_roots=("${scratch}"/extracted/*)
[[ ${#package_roots[@]} == 1 && -d ${package_roots[0]} ]]
# Hide the complete build tree while testing both installed forms.
mv build-cmake "${scratch}/build-cmake"
restore_build() { mv "${scratch}/build-cmake" "${root}/build-cmake"; }
trap restore_build EXIT
python3 tests/package/installed_profile.py --zsh "${ZSH_EXECUTABLE}" \
  --prefix "${scratch}/prefix" --report qualification-results/cmake-install.json
prefix=${package_roots[0]}
if [[ -d ${prefix}/usr ]]; then prefix=${prefix}/usr; fi
python3 tests/package/installed_profile.py --zsh "${ZSH_EXECUTABLE}" \
  --prefix "${prefix}" --report qualification-results/tgz-install.json
