#!/usr/bin/env zsh
# Native Zsh executable; compatibility floor 5.8.1.
emulate -R zsh
setopt err_exit pipe_fail
cd -- "${0:A:h:h}"
: ${ZD_OUTPUT_DIR:?Run through the controlled zd module-build profile}
: ${ZD_SOURCE_REVISION:?Missing checkout identity}
typeset runtime=${commands[zsh]:?Missing selected Zsh runtime}

# Generate the pinned vendored headers without fetching inside the container.
zsh -f scripts/cmake.configure.zsh --no-submodule --jobs 2
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release \
  -DZSH_EXECUTABLE="$runtime" -DZPMOD_VERSION="0.0.0+g${ZD_SOURCE_REVISION[1,12]}"
cmake --build build-cmake -j2
cmake --install build-cmake --prefix "$PWD/build-cmake/stage"
ctest --test-dir build-cmake --output-on-failure > "$ZD_OUTPUT_DIR/ctest.log" 2>&1 || {
  cat -- "$ZD_OUTPUT_DIR/ctest.log"
  exit 1
}
cat -- "$ZD_OUTPUT_DIR/ctest.log"
cp -- build-cmake/CTestTestfile.cmake "$ZD_OUTPUT_DIR/CTestTestfile.cmake"
mkdir -p -- "$ZD_OUTPUT_DIR/module"
cp -- build-cmake/stage/lib/zsh/site-modules/zpmod.so "$ZD_OUTPUT_DIR/module/"
if [[ ${1:-} == benchmark ]]; then
  : ${ZD_RUNNER_IMAGE:?Missing image identity}
  shift
  python3 benchmarks/compare.py --module-dir "$PWD/build-cmake/stage/lib/zsh/site-modules" \
    --source-revision "$ZD_SOURCE_REVISION" --runner-image "$ZD_RUNNER_IMAGE" \
    --output-dir "$ZD_OUTPUT_DIR/comparison" "$@"
fi
