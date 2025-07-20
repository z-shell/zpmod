# Scripts Directory

This directory contains utility scripts for building, installing, and maintaining the zpmod module.

## Installation Scripts

### `install.sh` - **End-User Installation**

**Target Audience**: End users, plugin managers, automated installations

```bash
# Basic installation
./Scripts/install.sh

# Advanced options
./Scripts/install.sh --target=/custom/path --verbose --no-git
```

**Features:**

- Simple, reliable installation process
- Automated dependency detection
- Integration with plugin managers
- Minimal configuration required
- Production-ready defaults

### `advanced-install.sh` - **Developer Installation**

**Target Audience**: Contributors, developers, power users

```bash
# Development installation with all features
./Scripts/advanced-install.sh --dev-mode --enable-debugging
```

**Features:**

- Development environment setup
- Advanced configuration options
- Debugging capabilities
- Custom build configurations
- Integration with development tools

## Version Management

### `bump-version.sh` - **Automated Version Management**

Manages zpmod's independent versioning system (separate from Zsh).

```bash
# Increment version types
./Scripts/bump-version.sh patch   # 1.0.0 → 1.0.1
./Scripts/bump-version.sh minor   # 1.0.0 → 1.1.0
./Scripts/bump-version.sh major   # 1.0.0 → 2.0.0

# Set specific version
./Scripts/bump-version.sh version 2.1.3-rc1

# Preview changes (dry run)
./Scripts/bump-version.sh --dry-run patch
```

**Automatically updates:**

- `Config/zpmod-version.mk` - All version components
- `Src/zi/zpmod.c` - C version constants
- `CHANGELOG.md` - Release notes section

## Maintenance Scripts

### `maintenance.sh` - **Workspace Health & Quality**

Comprehensive workspace maintenance utilities.

```bash
# Check overall workspace health
./Scripts/maintenance.sh check-health

# Run code quality checks
./Scripts/maintenance.sh lint-code

# Clean build artifacts
./Scripts/maintenance.sh clean-build

# Verify version consistency
./Scripts/maintenance.sh check-versions

# Basic security scanning
./Scripts/maintenance.sh security-scan
```

### Cleaning Operations

Cleaning functionality is integrated into the maintenance script:

```bash
# Deep clean of build artifacts and temporary files
./Scripts/maintenance.sh clean-deep

# Or as part of comprehensive maintenance
./Scripts/maintenance.sh comprehensive
```

## Development Utilities

### `copy_from_zsh_src.zsh` - **Zsh Source Integration**

Copies and adapts source files from Zsh codebase when updating zpmod's base functionality.
