# Changelog

All notable changes to the zpmod project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Comprehensive branching and tagging guidelines for version management
- Enhanced documentation structure following Divio Documentation System

### Changed

- Updated configuration paths to align with Zi plugin manager integration
- Improved Scripts/README.md with clear usage guidance for different installation methods

### Fixed

- Resolved workspace cleanup issues by removing deprecated CVS and legacy files
- Corrected configuration file paths to use ~/.config/zi/ for proper Zi integration

### Security

- Implemented comprehensive workspace modernization removing legacy security risks

## [5.9.0.1-dev] - Development Version

### Note

This is the current development version. All changes above will be included in the next release.

---

## Release Process

When creating a new release:

1. Move items from `[Unreleased]` to the new version section
2. Update the version number in `Config/version.mk`
3. Follow the [Branching and Tagging Guidelines](docs/how-to/branching-and-tagging-guidelines.md)
4. Create a new tag with the version number
5. Update this changelog and commit the changes

## Version Format

- **Major.Minor.Patch** format following [Semantic Versioning](https://semver.org/)
- **Added** for new features
- **Changed** for changes in existing functionality
- **Deprecated** for soon-to-be removed features
- **Removed** for now removed features
- **Fixed** for any bug fixes
- **Security** for vulnerability fixes

## Links

- [GitHub Releases](https://github.com/z-shell/zpmod/releases)
- [Installation Guide](docs/tutorials/getting-started.md)
- [Contributing Guidelines](docs/CONTRIBUTING.md)
