# zpmod Technical Requirements

## Project Overview

The `zpmod` project is a binary Zsh module that enhances Zsh functionality by:

- Transparently and automatically compiling sourced scripts
- Providing performance tracking for sourced files
- Handling special file paths like `/proc/self/fd/*`

## Technical Requirements

### Language Requirements

- **Primary Language**: C (89.1%)
- **Build System**: Autoconf/Automake (M4, 6.5%)
- **Scripts**: Shell/Zsh (3.2%)

### Platform Support

- **Linux**: Primary platform, uses `.so` module extension
- **macOS**: Secondary platform, uses `.bundle` module extension

### Zsh Compatibility

- Requires Zsh version 5.8.1 or newer
- Follows Zsh module API conventions

### Build Requirements

- GCC or compatible compiler
- GNU Make
- Autoconf/Automake tools

## Code Organization Requirements

### Src Directory

- Contains the C source code for the module
- Module code is in the `zi/` subdirectory
- Follows Zsh module coding conventions

### Config Directory

- Contains configuration templates
- Version information in `version.mk`

### Scripts Directory

- Contains build and installation scripts
- User-facing utility scripts

### Test Directory

- Contains test suite using Zsh test framework
- Tests should verify module functionality

### Error Handling Requirements

- Proper handling of file descriptors
- Skip compilation for special files
- Graceful error reporting
