# zpmod Versioning Architecture

## Overview

This document explains the versioning system for the zpmod module, which maintains **independent versioning separate from Zsh**.

## Version Files

### `Config/zpmod-version.mk`

**The authoritative source for zpmod version information.**

```makefile
# zpmod Version Information
ZPMOD_VERSION=1.0.0-dev
ZPMOD_VERSION_DATE='July 20, 2025'
ZPMOD_VERSION_MAJOR=1
ZPMOD_VERSION_MINOR=0
ZPMOD_VERSION_PATCH=0
ZPMOD_VERSION_PRERELEASE=dev
```

### `Config/version.mk`

**Contains Zsh version - DO NOT MODIFY for zpmod releases.**

```makefile
VERSION=5.9.0.1-dev
VERSION_DATE='May 15, 2022'
```

### `Src/zi/zpmod.c`

**C source constants automatically synchronized with zpmod-version.mk.**

```c
#define ZPMOD_VERSION "1.0.0-dev"
#define ZPMOD_VERSION_MAJOR 1
#define ZPMOD_VERSION_MINOR 0
#define ZPMOD_VERSION_PATCH 0
#define ZPMOD_VERSION_PRERELEASE "dev"
```

## Version Management Workflow

### Automated Version Updates

Use the `Scripts/bump-version.sh` script for all version changes:

```bash
# Increment patch version (1.0.0 → 1.0.1)
./Scripts/bump-version.sh patch

# Increment minor version (1.0.0 → 1.1.0)
./Scripts/bump-version.sh minor

# Increment major version (1.0.0 → 2.0.0)
./Scripts/bump-version.sh major

# Set specific version
./Scripts/bump-version.sh version 2.1.3-rc1

# Dry run (preview changes)
./Scripts/bump-version.sh --dry-run patch
```

### What Gets Updated

The script automatically updates:

1. **`Config/zpmod-version.mk`** - All version components
2. **`Src/zi/zpmod.c`** - C version constants
3. **`CHANGELOG.md`** - Adds/updates Unreleased section

### Version Checking

Users can check the zpmod version at runtime:

```zsh
# Load the module
zmodload zi/zpmod

# Check version
zpmod version
```

Output:

```text
zpmod version 1.0.0-dev
  Major: 1
  Minor: 0
  Patch: 0
  Pre-release: dev
```

## Integration Points

### Build System

- `Src/Makefile` includes `Config/zpmod-version.mk`
- Version constants compiled into the module

### CI/CD Pipeline

- GitHub Actions uses git tags (v\*) for releases
- Independent of Zsh version tracking

### Documentation

- All documentation references `Config/zpmod-version.mk`
- Clear separation from Zsh versioning

## Key Principles

1. **Independence**: zpmod version is completely separate from Zsh version
2. **Automation**: Use Scripts/bump-version.sh for all version changes
3. **Consistency**: Version appears in multiple files but managed from single source
4. **Visibility**: Users can query version at runtime via `zpmod version`
