# Scripts Directory

This directory contains various utility scripts for building, installing, and maintaining the zpmod project.

## Available Scripts

### Installation Scripts

- **install.sh** - Traditional build script for developers and build systems
  - Source compilation using autoconf/make workflow
  - Supports build customization (`--cflags`, `--target`, `--clean`)
  - Git repository management and branch selection
  - **Use when**: Building from source, development, CI/CD, custom configurations

- **advanced-install.sh** - Comprehensive installation manager for end users
  - **Multiple installation types**: binary downloads, source compilation, development setup
  - **Zi plugin manager integration**: automatic configuration and shell setup
  - **User-friendly**: platform detection, automatic dependencies, verification
  - **Use when**: Quick setup, production use, Zi ecosystem integration

### Utility Scripts

- **clean.sh** - Cleans up build artifacts and temporary files
  - Removes object files, shared libraries, and other generated files
  - Use with `--verbose` to see all commands being executed

- **copy_from_zsh_src.zsh** - Updates source files from a Zsh source tree
  - Used for syncing with newer versions of Zsh
  - Primarily for development and maintenance

- **update-readme.sh** - Maintains the root README.md based on docs content
  - Automatically extracts key information from documentation files
  - Options: `--check-only` to verify without making changes, `--verbose` for detailed output
  - Used by the GitHub Actions workflow to keep docs in sync

## Usage

Most scripts support a `--help` or `-h` option to show usage information.

### Quick Start Guide

**For most users (recommended):**

```bash
./Scripts/advanced-install.sh
```

**For developers or custom builds:**

```bash
./Scripts/install.sh --help  # See all options
./Scripts/install.sh --target ~/.local --verbose
```

**For Zi plugin manager users:**

```bash
./Scripts/advanced-install.sh --zi --type source
```

For detailed usage, see the main README.md file in the repository root.
