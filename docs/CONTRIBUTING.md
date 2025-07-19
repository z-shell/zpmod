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

### Documentation Workflow

The repository uses a documentation-driven approach with the following guidelines:

1. **Documentation Structure**:
   - Detailed documentation lives in the `docs/` directory
   - The root `README.md` provides a high-level overview with links to detailed docs

2. **Keeping Documentation in Sync**:
   - When updating documentation in the `docs/` directory, run `./Scripts/update-readme.sh`
   - This script automatically updates the root README.md with key information from docs
   - A GitHub Actions workflow (`sync-docs.yml`) automatically keeps the README.md in sync

3. **Documentation Files**:
   - `GUIDE.md`: User installation and usage instructions
   - `API.md`: Technical API reference
   - `IMPROVEMENTS.md`: Recent and planned technical improvements
   - `CONTRIBUTING.md`: This guide for contributors
   - `index.md`: Main documentation entry point

### Code Style

- Follow the existing code style in the project
- Use descriptive variable and function names
- Add comments for complex logic
- Keep functions focused on a single responsibility

### Commit Guidelines

- Use clear, descriptive commit messages
- Reference issue numbers in commit messages when applicable
- Keep commits focused on a single change

## Pull Request Process

1. **Create a branch**: Create a branch for your changes
2. **Make your changes**: Implement your changes, following the code style guidelines
3. **Test your changes**: Ensure that your changes pass all tests
4. **Submit a pull request**: Submit a pull request from your fork to the main repository
5. **Address review comments**: Respond to any review comments and make necessary changes

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
