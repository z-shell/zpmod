# Branching and Tagging Guidelines

This document provides comprehensive guidelines for version management, branching strategy, and release processes for the zpmod project.

## Table of Contents

- [Overview](#overview)
- [Branching Strategy](#branching-strategy)
- [Version Numbering](#version-numbering)
- [Tagging Guidelines](#tagging-guidelines)
- [Release Process](#release-process)
- [Hotfix Process](#hotfix-process)
- [Development Workflow](#development-workflow)
- [Automation and CI/CD](#automation-and-cicd)
- [Best Practices](#best-practices)

## Overview

The zpmod project follows a **Git Flow-inspired** branching model adapted for the Z-Shell ecosystem, emphasizing stability, automated testing, and clear version management.

### Core Principles

- **Stability**: `main` branch is always deployable
- **Predictability**: Clear version numbering and release cycles
- **Quality**: All changes go through testing and review
- **Automation**: CI/CD handles testing, building, and releases
- **Documentation**: All releases include comprehensive changelogs

## Branching Strategy

### Branch Types

#### 1. **Main Branch** (`main`)

- **Purpose**: Stable, production-ready code
- **Protection**: Requires PR reviews, passing CI
- **Version**: Always contains the latest stable release
- **Deployment**: Automatically triggers releases when tagged

#### 2. **Development Branch** (`develop`)

- **Purpose**: Integration branch for next release
- **Source**: Feature branches merge here first
- **Testing**: Continuous integration and compatibility testing
- **Stability**: Should be stable but may contain unreleased features

#### 3. **Feature Branches** (`feature/ISSUE-brief-description`)

- **Purpose**: Individual features or enhancements
- **Naming**: `feature/31-fix-file-descriptor-error`
- **Lifecycle**: Branch from `develop`, merge back to `develop`
- **Testing**: Must pass all CI checks before merge

#### 4. **Release Branches** (`release/v1.2.0`)

- **Purpose**: Preparation for new releases
- **Source**: Branch from `develop` when feature-complete
- **Target**: Merge to both `main` and `develop`
- **Activities**: Version bumping, documentation updates, final testing

#### 5. **Hotfix Branches** (`hotfix/v1.1.1-critical-fix`)

- **Purpose**: Critical fixes for production issues
- **Source**: Branch from `main`
- **Target**: Merge to both `main` and `develop`
- **Urgency**: Bypass normal release cycle for critical issues

### Branch Naming Conventions

```bash
# Feature branches
feature/ISSUE-brief-description
feature/45-improve-performance-tracking
feature/67-add-memory-optimization

# Release branches
release/v1.2.0
release/v2.0.0-beta.1

# Hotfix branches
hotfix/v1.1.1-security-fix
hotfix/v1.2.1-compilation-error

# Maintenance branches (for long-term support)
maint/v1.x
maint/v2.x
```

## Version Numbering

### Semantic Versioning (SemVer)

zpmod follows **Semantic Versioning 2.0.0** with Zsh ecosystem adaptations:

```text
MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]
```

#### Version Components

- **MAJOR** (`X.0.0`): Breaking changes, API incompatibilities
- **MINOR** (`0.X.0`): New features, backward-compatible additions
- **PATCH** (`0.0.X`): Bug fixes, security patches, backward-compatible
- **PRERELEASE** (`-alpha.1`, `-beta.2`, `-rc.1`): Development versions
- **BUILD** (`+20250120.1`): Build metadata (optional)

#### Examples

```bash
# Stable releases
v1.0.0      # Initial stable release
v1.1.0      # New features added
v1.1.1      # Bug fixes
v2.0.0      # Breaking changes (API changes)

# Pre-releases
v1.2.0-alpha.1    # Alpha version
v1.2.0-beta.1     # Beta version
v1.2.0-rc.1       # Release candidate
v2.0.0-dev        # Development version
```

### Version File Management

#### Primary Version Sources

> **⚠️ IMPORTANT**: zpmod has independent versioning separate from Zsh

1. **`Config/zpmod-version.mk`** - zpmod version definition
2. **`Config/version.mk`** - Zsh version (do not modify for zpmod releases)
3. **Git tags** - Source of truth for releases
4. **`Src/zi/zpmod.c`** - C version constants (auto-updated by Scripts/bump-version.sh)

#### Version Update Process

```bash
# 1. Update zpmod version using automated script
./Scripts/bump-version.sh minor  # or patch, major, or specific version

# 2. Update any version references in documentation
grep -r "v1.1.0" docs/ # Find old version references

# 3. Commit version bump
git add Config/zpmod-version.mk docs/
git commit -m "bump: version 1.1.0 → 1.2.0"

# 4. Create and push tag
git tag -a v1.2.0 -m "Release version 1.2.0"
git push origin v1.2.0
```

## Tagging Guidelines

### Tag Naming Convention

```bash
# Release tags
v1.0.0          # Stable release
v1.2.0-beta.1   # Pre-release
v2.0.0-rc.1     # Release candidate

# Development tags (avoid in main repo)
nightly-20250120    # Nightly builds (CI only)
```

### Tag Creation Process

#### 1. **Prepare Release**

```bash
# Create release branch
git checkout develop
git pull origin develop
git checkout -b release/v1.2.0

# Update version
echo "ZPMOD_VERSION=1.2.0" > Config/zpmod-version.mk
git add Config/zpmod-version.mk
git commit -m "bump: version 1.1.0 → 1.2.0"

# Update CHANGELOG (see Release Process)
# Test thoroughly
# Update documentation
```

#### 2. **Create Annotated Tag**

```bash
# Merge to main
git checkout main
git merge --no-ff release/v1.2.0

# Create annotated tag with detailed message
git tag -a v1.2.0 -m "Release version 1.2.0

## New Features
- Enhanced performance tracking with memory optimization
- Added lazy loading support for improved startup time
- New configuration helpers for easier setup

## Bug Fixes
- Fixed file descriptor leak in module loading
- Resolved compilation errors on macOS ARM64
- Corrected path resolution in Zi integration

## Breaking Changes
- None

## Migration Guide
No migration required for this release.

Full changelog: https://github.com/z-shell/zpmod/blob/v1.2.0/CHANGELOG.md"

# Push tag
git push origin v1.2.0
```

#### 3. **Tag Verification**

```bash
# Verify tag exists
git tag -l "v1.2.0"

# Check tag details
git show v1.2.0

# Verify tag points to correct commit
git rev-parse v1.2.0
git rev-parse HEAD
```

### Tag Management

#### List and Filter Tags

```bash
# List all tags
git tag -l

# List release tags only
git tag -l "v*"

# List tags with pattern
git tag -l "v1.*"

# Show tag details
git show v1.2.0
```

#### Delete Tags (if needed)

```bash
# Delete local tag
git tag -d v1.2.0

# Delete remote tag
git push origin --delete v1.2.0

# Recreate corrected tag
git tag -a v1.2.0 -m "Corrected release message"
git push origin v1.2.0
```

## Release Process

### 1. **Pre-Release Preparation**

#### Create CHANGELOG.md (if not exists)

```bash
# Create comprehensive changelog
touch CHANGELOG.md
```

#### Update Documentation

```bash
# Update version references in documentation
find docs/ -name "*.md" -exec grep -l "v1.1.0" {} \;
# Update each file with new version

# Update installation examples
# Update API documentation
# Update tutorial versions
```

### 2. **Release Branch Workflow**

```bash
# 1. Ensure develop is up to date
git checkout develop
git pull origin develop

# 2. Create release branch
git checkout -b release/v1.2.0

# 3. Version bump and changelog
echo "ZPMOD_VERSION=1.2.0" > Config/zpmod-version.mk
# Update CHANGELOG.md with release notes
git add .
git commit -m "prepare: release v1.2.0"

# 4. Push and create PR to main
git push origin release/v1.2.0
# Create PR: release/v1.2.0 → main
```

### 3. **Release Execution**

```bash
# After PR is approved and merged to main
git checkout main
git pull origin main

# Create and push tag
git tag -a v1.2.0 -m "Release version 1.2.0 - See CHANGELOG.md"
git push origin v1.2.0

# Merge back to develop
git checkout develop
git merge --no-ff main
git push origin develop

# Clean up release branch
git branch -d release/v1.2.0
git push origin --delete release/v1.2.0
```

### 4. **Post-Release Tasks**

- **GitHub Release**: Created automatically by CI from tag
- **Documentation**: Verify docs are updated
- **Announcements**: Update project README, notify users
- **Monitoring**: Watch for issues with new release

## Hotfix Process

For critical issues requiring immediate release:

### 1. **Create Hotfix Branch**

```bash
# Branch from main (current production)
git checkout main
git pull origin main
git checkout -b hotfix/v1.1.1-critical-security-fix
```

### 2. **Implement Fix**

```bash
# Make minimal changes to fix the issue
# Write tests for the fix
# Update version number
echo "ZPMOD_VERSION=1.1.1" > Config/zpmod-version.mk

# Commit with descriptive message
git add .
git commit -m "fix: critical security vulnerability in file parsing

- Fixes CVE-2025-XXXX buffer overflow
- Adds input validation for zpmod commands
- Updates documentation with security note

Fixes #123"
```

### 3. **Release Hotfix**

```bash
# Create PR to main for review
git push origin hotfix/v1.1.1-critical-security-fix
# Fast-track review process

# After merge to main
git checkout main
git pull origin main

# Tag and release
git tag -a v1.1.1 -m "Hotfix v1.1.1: Critical security fix

SECURITY: Fixes buffer overflow vulnerability in zpmod command parsing.
All users should upgrade immediately.

Details: https://github.com/z-shell/zpmod/security/advisories/GHSA-XXXX"

git push origin v1.1.1

# Merge to develop
git checkout develop
git merge --no-ff main
git push origin develop
```

## Development Workflow

### Feature Development

```bash
# 1. Create feature branch from develop
git checkout develop
git pull origin develop
git checkout -b feature/45-improve-performance-tracking

# 2. Implement feature with tests
# Write code
# Add tests
# Update documentation

# 3. Commit with conventional format
git add .
git commit -m "feat: add memory usage tracking to performance analysis

- Implements real-time memory monitoring
- Adds memory profiling to zpmod-stats
- Updates configuration with memory options
- Includes comprehensive test coverage

Closes #45"

# 4. Push and create PR
git push origin feature/45-improve-performance-tracking
# Create PR: feature/45-improve-performance-tracking → develop
```

### Commit Message Format

Follow **Conventional Commits** specification:

```text
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

#### Commit Types

- **feat**: New features
- **fix**: Bug fixes
- **docs**: Documentation changes
- **style**: Code style changes (no logic change)
- **refactor**: Code refactoring
- **test**: Adding or updating tests
- **chore**: Build process, dependencies, tools
- **perf**: Performance improvements
- **ci**: CI/CD changes
- **bump**: Version bumps

#### Commit Message Examples

```bash
feat(config): add lazy loading support for zpmod initialization

fix(core): resolve file descriptor leak in module loading
- Ensures all file descriptors are properly closed
- Adds cleanup in error handling paths
- Includes regression test

docs: update installation guide with new Zi integration steps

bump: version 1.1.0 → 1.2.0

ci: add automated security scanning to release pipeline
```

## Automation and CI/CD

### GitHub Actions Integration

The project uses automated workflows for:

#### 1. **Continuous Integration** (`.github/workflows/ci.yml`)

- **Trigger**: Push to any branch, PRs
- **Tasks**: Build, test, security scan
- **Platforms**: Linux (Ubuntu), macOS
- **Zsh Versions**: 5.8, 5.9, latest

#### 2. **Release Automation** (`.github/workflows/release.yml`)

- **Trigger**: Tags matching `v*`
- **Tasks**: Build binaries, create GitHub release
- **Artifacts**: Module files for Linux and macOS
- **Documentation**: Auto-generated release notes

#### 3. **Security Scanning** (`.github/workflows/codeql.yml`)

- **Trigger**: Weekly, PRs to main
- **Tools**: CodeQL, dependency scanning
- **Reports**: Security advisories

### Release Automation Features

```yaml
# Automated release creation
on:
  push:
    tags:
      - "v*"

# Build matrix for multiple platforms
strategy:
  matrix:
    os: [ubuntu-latest, macos-latest]

# Automatic changelog generation
- name: Generate Changelog
  uses: mikepenz/release-changelog-builder-action@v3
```

### Protected Branch Rules

Configure branch protection for `main`:

```yaml
# Required settings
- Require pull request reviews (2 reviewers)
- Require status checks to pass
- Require branches to be up to date
- Restrict pushes to specific users/teams
- Allow force pushes: false
- Allow deletions: false
```

## Best Practices

### For Maintainers

#### Version Planning

1. **Plan releases** around Zsh version cycles
2. **Group features** into logical releases
3. **Communicate breaking changes** well in advance
4. **Maintain compatibility** with supported Zsh versions

#### Quality Assurance

```bash
# Pre-release checklist
□ All tests passing on CI
□ Documentation updated
□ CHANGELOG.md updated
□ Version numbers consistent
□ Security scan passed
□ Backward compatibility verified
□ Performance benchmarks stable
```

#### Release Communication

1. **GitHub Releases**: Detailed release notes
2. **README Updates**: Latest version information
3. **Documentation**: Migration guides for breaking changes
4. **Community**: Announcements in Z-Shell community channels

### For Contributors

#### Before Creating a Feature Branch

```bash
# Ensure you have latest changes
git checkout develop
git pull origin develop

# Check for existing work
git branch -r | grep feature/your-topic
```

#### Keeping Feature Branches Updated

```bash
# Regularly sync with develop
git checkout feature/your-branch
git fetch origin
git merge origin/develop

# Or use rebase for cleaner history
git rebase origin/develop
```

#### Preparing for Review

```bash
# Run full test suite
make test

# Check code style
# Run linting tools
# Update documentation
# Write clear commit messages
```

### For Users

#### Staying Updated

```bash
# Check current version
zpmod --version

# Check for updates
git fetch --tags origin
git tag -l "v*" | sort -V | tail -1

# Upgrade using specific version
./Scripts/advanced-install.sh --type binary
```

#### Reporting Issues

When reporting version-specific issues:

1. **Include version**: `zpmod --version`
2. **Include Zsh version**: `zsh --version`
3. **Include OS information**: `uname -a`
4. **Test with latest version** before reporting

## Troubleshooting

### Common Issues

#### Version Mismatch

```bash
# Check version consistency
grep ZPMOD_VERSION Config/zpmod-version.mk
git describe --tags
zpmod --version

# Fix inconsistencies
git checkout main
git pull origin main
./Scripts/install.sh --clean
```

#### Tag Issues

```bash
# Recreate corrupted tag
git tag -d v1.2.0
git push origin --delete v1.2.0
git tag -a v1.2.0 HEAD
git push origin v1.2.0
```

#### Branch Cleanup

```bash
# Clean up merged feature branches
git branch --merged develop | grep feature/ | xargs git branch -d

# Clean up remote tracking branches
git remote prune origin
```

## References

- [Semantic Versioning](https://semver.org/)
- [Git Flow](https://nvie.com/posts/a-successful-git-branching-model/)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [GitHub Flow](https://guides.github.com/introduction/flow/)
- [Z-Shell Organization Guidelines](https://github.com/z-shell/.github)

---

_This document is maintained by the zpmod project maintainers. For questions or suggestions, please [open an issue](https://github.com/z-shell/zpmod/issues/new)._
