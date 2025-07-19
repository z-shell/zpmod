# Scripts Directory

This directory contains various utility scripts for building, installing, and maintaining the zpmod project.

## Available Scripts

- **install.sh** - Main installation and build script for compiling the zpmod module

  - Supports various command-line options (run with `--help` to see all options)
  - Handles configuration, compilation, and installation
  - This is the recommended script for most users

- **clean.sh** - Cleans up build artifacts and temporary files

  - Removes object files, shared libraries, and other generated files
  - Use with `--verbose` to see all commands being executed

- **copy_from_zsh_src.zsh** - Updates source files from a Zsh source tree
  - Used for syncing with newer versions of Zsh
  - Primarily for development and maintenance

## Usage

Most scripts support a `--help` or `-h` option to show usage information.

For typical usage, see the main README.md file in the repository root.
