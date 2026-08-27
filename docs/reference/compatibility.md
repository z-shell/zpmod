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
