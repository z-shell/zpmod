# Contributing to zpmod

## Introduction

Thank you for your interest in contributing to zpmod! This document provides guidelines and instructions for contributing to the project.

## Getting Started

1. **Fork the repository**: Start by forking the [zpmod repository](https://github.com/z-shell/zpmod)
2. **Clone your fork**: `git clone https://github.com/YOUR-USERNAME/zpmod.git`
3. **Set up the development environment**: Follow the installation instructions in the README.md

## Development Workflow

### Building From Source

```zsh
# Configure the build
./configure

# Build the module
make

# Run tests
make test
```

### Quality Assurance with Trunk

The zpmod project uses [trunk.io](https://trunk.io) for comprehensive code quality management. All contributors should use trunk for consistent quality checks and automated code formatting.

#### Quick Start with Trunk

```bash
# Install trunk (first time only)
curl https://get.trunk.io -fsSL | bash

# Run all quality checks (recommended for most development)
trunk check -y

# Run zpmod-specific quality checks only
trunk check --filter=zpmod-quality

# Run maintenance checks (cleaning, version validation)
trunk check --filter=zpmod-maintenance

# Format code automatically
trunk fmt
```

#### Development Workflow with Trunk

1. **Start development**: `trunk check --filter=zpmod-maintenance` (health check)
2. **During development**: `trunk check --filter=zpmod-quality --sample=10` (quick feedback)
3. **Before commit**: `trunk check -y` (full validation)
4. **Pre-PR submission**: `trunk check` (comprehensive scan)

#### Quality Checks Available

The zpmod trunk configuration includes specialized linters:

- **zpmod-quality**: TODO detection, build consistency, documentation structure, link validation
- **zpmod-maintenance**: Health checks, version validation, workspace cleaning

#### Alternative: Manual Quality Scripts

If you prefer running quality checks individually:

```bash
# Individual quality checks
./Scripts/quality-todo-check.sh
./Scripts/quality-build-check.sh
./Scripts/quality-docs-structure.sh
./Scripts/quality-docs-links.sh

# Comprehensive maintenance
./Scripts/maintenance.sh comprehensive
```

### Documentation Workflow

The repository follows the **Divio Documentation System** for comprehensive and well-organized documentation:

#### Documentation Structure

```text
docs/
├── tutorials/          # Learning-oriented (hands-on lessons)
├── how-to/             # Problem-oriented (practical guides)
├── reference/          # Information-oriented (technical specs)
├── explanation/        # Understanding-oriented (background knowledge)
├── index.md            # Main documentation hub
└── CONTRIBUTING.md     # This contribution guide
```

#### Documentation Categories

1. **`tutorials/`** - Step-by-step learning guides for beginners
2. **`how-to/`** - Task-oriented solutions to specific problems
3. **`reference/`** - Technical specifications and API documentation
4. **`explanation/`** - Conceptual guides and architectural discussions

#### Contributing to Documentation

When adding or updating documentation:

1. **Determine the correct category** based on content type
2. **Place files in appropriate directory** (`tutorials/`, `how-to/`, `reference/`, `explanation/`)
3. **Use descriptive, kebab-case filenames** (e.g., `trunk-workflow-guide.md`)
4. **Update `docs/index.md`** to link new documentation
5. **Run trunk checks** to validate documentation structure:
   ```bash
   trunk check --filter=zpmod-quality  # Validates docs structure and links
   ```

#### Documentation Quality Checks

The trunk configuration includes specialized documentation linters:

- **Documentation structure validation**: Ensures proper Divio categorization
- **Link validation**: Checks for broken internal and external links
- **Consistency checks**: Verifies navigation and cross-references

## Coding Standards

- Follow the existing code style in the project
- Use descriptive variable and function names
- Add comments for complex logic
- Keep functions focused on a single responsibility
- **Run `trunk fmt` before committing** to ensure consistent formatting
- **Use `trunk check -y` to validate code quality** before submitting PRs

### Code Quality Requirements

All code must pass trunk quality checks before merge:

```bash
# Essential pre-commit check
trunk check -y

# This runs all configured linters including:
# - C code formatting and style checks
# - Documentation validation
# - Build system consistency
# - TODO/FIXME detection
# - Security scanning
```

### Commit Guidelines

- Use clear, descriptive commit messages
- Reference issue numbers in commit messages when applicable
- Keep commits focused on a single change

## Pull Request Process

1. **Create a branch**: Create a branch for your changes
2. **Make your changes**: Implement your changes, following the code style guidelines
3. **Run quality checks**: Execute `trunk check -y` to ensure code quality
4. **Test your changes**: Ensure that your changes pass all tests (`make test`)
5. **Submit a pull request**: Submit a pull request from your fork to the main repository
6. **Address review comments**: Respond to any review comments and make necessary changes

### Pre-PR Checklist

Before submitting your pull request, ensure:

- [ ] `trunk check -y` passes without errors
- [ ] `make test` passes all tests
- [ ] Documentation is updated (if applicable)
- [ ] Commit messages are clear and descriptive
- [ ] No TODO/FIXME items remain (unless explicitly documented)

### Automated CI Checks

Our GitHub Actions workflow will automatically run:

- Comprehensive trunk quality checks
- Security vulnerability scans
- Documentation validation
- Build system verification

All checks must pass before merge.

## Reporting Bugs

When reporting bugs, please include:

1. The version of zpmod you're using
2. Your operating system and Zsh version
3. Steps to reproduce the bug
4. Expected behavior
5. Actual behavior

## Feature Requests

Feature requests are welcome! Please provide:

1. A clear description of the feature
2. The use case for the feature
3. Any relevant examples or mockups

## Code of Conduct

Please be respectful and considerate of others when contributing to the project. We strive to maintain a welcoming and inclusive environment for all contributors.
