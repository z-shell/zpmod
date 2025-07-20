# Trunk-Based Development Workflow Guide

This guide provides comprehensive instructions for using the new trunk.io-based development workflow in the zpmod project.

## Overview

We've migrated from traditional GitHub Actions to a unified trunk.io-based quality system that provides:

- **Faster feedback**: ~5.7s for complete quality checks vs. previous multi-minute workflows
- **Consistent tooling**: Single command for all quality checks
- **Better developer experience**: Real-time feedback during development
- **Comprehensive coverage**: Code quality, security, documentation, and build validation

## Installation

### First-Time Setup

```bash
# Install trunk CLI
curl https://get.trunk.io -fsSL | bash

# Navigate to zpmod project
cd /path/to/zpmod

# Verify installation
trunk --version
```

### IDE Integration

For VS Code users, install the Trunk extension:

- Open VS Code
- Go to Extensions (Ctrl+Shift+X)
- Search for "Trunk"
- Install the official Trunk extension

## Daily Development Workflow

### 1. Start of Development Session

```bash
# Health check and workspace validation
trunk check --filter=zpmod-maintenance

# This validates:
# - Version consistency across files
# - Build system health
# - Workspace cleanliness
```

### 2. During Active Development

```bash
# Quick quality feedback (samples subset of files)
trunk check --filter=zpmod-quality --sample=10

# This checks:
# - TODO/FIXME items
# - Build system consistency
# - Documentation structure
# - Link validation
```

### 3. Before Committing

```bash
# Comprehensive validation (all files)
trunk check -y

# This runs ALL configured linters:
# - Code formatting and style
# - Security scanning
# - Documentation validation
# - Build system checks
# - TODO detection
```

### 4. Code Formatting

```bash
# Auto-format all supported files
trunk fmt

# Format specific files
trunk fmt src/module.c docs/README.md
```

## Command Reference

### Essential Commands

| Command                                  | Purpose                     | When to Use                       |
| ---------------------------------------- | --------------------------- | --------------------------------- |
| `trunk check -y`                         | Full quality validation     | Before commits, PR submission     |
| `trunk check --filter=zpmod-quality`     | Code quality checks only    | During development                |
| `trunk check --filter=zpmod-maintenance` | Health and maintenance      | Start of session, troubleshooting |
| `trunk fmt`                              | Auto-format code            | Before commits                    |
| `trunk check --sample=10`                | Quick validation (10 files) | Rapid feedback during coding      |

### Advanced Commands

```bash
# Check specific files only
trunk check src/module.c docs/README.md

# Run with verbose output
trunk check --verbose

# Check all files (override sampling)
trunk check --all

# Show what would be checked without running
trunk check --dry-run

# Update trunk and linters
trunk upgrade
```

## Quality Check Categories

### zpmod-quality Linter

Focuses on code quality and project consistency:

1. **TODO Detection** (`quality-todo-check.sh`)
   - Finds TODO, FIXME, XXX, HACK comments
   - Ensures no unfinished work in releases

2. **Build Consistency** (`quality-build-check.sh`)
   - Validates .c/.syms file pairs
   - Ensures build system integrity

3. **Documentation Structure** (`quality-docs-structure.sh`)
   - Validates Divio documentation system
   - Ensures proper categorization

4. **Link Validation** (`quality-docs-links.sh`)
   - Detects broken markdown links
   - Validates internal references

### zpmod-maintenance Linter

Focuses on workspace health and maintenance:

1. **Health Check**
   - Version consistency validation
   - Build system verification

2. **Version Check**
   - Cross-file version synchronization
   - Release preparation validation

3. **Workspace Cleaning**
   - Removes build artifacts
   - Cleans temporary files

## Troubleshooting

### Common Issues

#### "Trunk not found" Error

```bash
# Reinstall trunk
curl https://get.trunk.io -fsSL | bash
# Restart your shell
exec $SHELL
```

#### Quality Check Failures

```bash
# Get detailed error information
trunk check --verbose

# Check specific failing linter
trunk check --filter=zpmod-quality --verbose

# Clean workspace and retry
trunk check --filter=zpmod-maintenance
```

#### Performance Issues

```bash
# Use sampling for faster feedback
trunk check --sample=10

# Check only modified files
trunk check --upstream-ref=main
```

### Getting Help

```bash
# Show all available commands
trunk help

# Show help for specific command
trunk help check

# Show available linters
trunk config linters
```

## Migration from Old Workflow

### What Changed

| Old Workflow                   | New Workflow                   |
| ------------------------------ | ------------------------------ |
| Multiple separate tools        | Single `trunk check` command   |
| 8 separate GitHub Actions jobs | 4 streamlined trunk-based jobs |
| ~3-5 minute CI runs            | ~1-2 minute CI runs            |
| Manual script execution        | Automated trunk integration    |

### Updated Commands

| Old Command                             | New Command                              |
| --------------------------------------- | ---------------------------------------- |
| `./Scripts/maintenance.sh lint-code`    | `trunk check --filter=zpmod-quality`     |
| `./Scripts/maintenance.sh check-health` | `trunk check --filter=zpmod-maintenance` |
| Multiple quality scripts                | `trunk check -y`                         |
| Manual formatting                       | `trunk fmt`                              |

## Best Practices

### Development Workflow

1. **Start each session** with `trunk check --filter=zpmod-maintenance`
2. **During development** use `trunk check --filter=zpmod-quality --sample=10` for quick feedback
3. **Before committing** always run `trunk check -y`
4. **Use `trunk fmt`** regularly to maintain consistent formatting

### Performance Optimization

- Use `--sample=N` for faster checks during development
- Use `--filter=specific-linter` to focus on relevant checks
- Run full checks (`trunk check -y`) only when necessary

### Team Collaboration

- Ensure everyone uses the same trunk version (`trunk upgrade`)
- Share trunk configuration changes through git
- Use consistent commands across the team

## CI/CD Integration

The new GitHub Actions workflow automatically runs trunk checks:

- **Comprehensive checks**: Full quality validation
- **Security scanning**: Vulnerability detection
- **Documentation validation**: Structure and link checks
- **Performance monitoring**: Tracks improvement metrics

### Local vs CI Behavior

- **Local**: Can use sampling and filtering for speed
- **CI**: Always runs comprehensive checks for reliability
- **Both**: Use identical trunk configuration for consistency

## Performance Benefits

Compared to the previous GitHub Actions workflow:

- **Execution time**: ~5.7s vs 3-5 minutes
- **Resource usage**: Single process vs multiple containers
- **Consistency**: Identical local and CI behavior
- **Maintainability**: Single configuration file vs multiple workflow files

## Next Steps

1. **Install trunk** following the setup instructions
2. **Practice the workflow** with a small change
3. **Integrate with your IDE** for real-time feedback
4. **Share feedback** with the team on workflow improvements

For questions or issues, please open a GitHub issue or discuss in team channels.
