# Compatibility

## Zsh version floor

The minimum supported Zsh release is **5.8.1**. The minimum-version container asserts that exact runtime before it builds the module and
runs the complete CTest suite. Native Linux and macOS jobs also run the full suite against their current Zsh environments.

A scheduled compatibility workflow builds the candidate revision through the latest Zi default branch, records the exact Zi revision, and
loads the resulting `zi/zpmod` module. This integration is a compatibility probe, not a dependency pin for end users.

## Prebuilt-package ABI policy

`zpmod` does not promise a universal Zsh module ABI across Zsh releases, operating systems, C libraries, or processor architectures. A
prebuilt package is supported only for the platform, architecture, and Zsh combinations exercised by the release and compatibility workflows
for that package revision.

The Zsh 5.8.1 source-compatibility floor does not imply that a binary built for another Zsh or platform combination will load safely. If a
published package does not match an exercised combination, build `zpmod` from source on the target system.

## Package-installation coverage

The full CTest suite generates a TGZ package, extracts it into an isolated prefix, loads the packaged module, verifies the `zpmod` builtin,
and confirms that the packaged `_zpmod` completion is autoloadable. This checks the installed layout rather than relying only on a
source-tree build.

## Explicit runtime qualification

`Runtime Qualification` exercises exact Zsh 5.8.1 and the explicitly patched `5.9.2+trap-bounds-a3547fd4` profile on native Linux and macOS.
The latter contains the upstream numeric trap bounds correction tracked in [#122](https://github.com/z-shell/zpmod/issues/122); it does not
qualify stock 5.9.2. Runtime setup is pinned independently from the zpmod candidate. The vendored header revision remains separately
recorded, because a runtime version alone does not identify the module ABI.

Each native cell runs the full existing CTest suite, then tests clean CMake and TGZ installations with the build directory hidden. Installed
payload checks include completion and license files and a real `source-study --json` event. A fresh Zi installation uses the exact revision
pinned in `scripts/qualify-zi.sh`, builds the checked-out candidate through Zi, and verifies its module and profiling output. The scheduled
latest-Zi probe remains independent.

Disposable Ubuntu and Fedora jobs additionally build, install, test and remove DEB and RPM packages with their native package managers.
These jobs record their distribution runtime and package identity separately; they establish only those exercised package/runtime
combinations. TGZ extraction does not imply DEB/RPM installation support.

Qualification artifacts retain runtime provenance, candidate/header revisions, installed-module hashes, package identities and test logs.
Interpret `source-study` JSON as instrumented diagnostic evidence, separate from benchmark workload reports. Performance timings remain
advisory. Native-oracle rollout, organization required checks and release publication are separate decisions; restore a previous reviewed
workflow/action pin to roll back a qualification change.
