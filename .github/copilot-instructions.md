---
applyTo: "**"
---

# GitHub Copilot Instructions for zpmod Repository

> **Repository**: [z-shell/zpmod](https://github.com/z-shell/zpmod)
> **Organization**: [Z-Shell](https://github.com/z-shell)
> **Last Updated**: 2025-07-19

## Project Overview

**zpmod** is a high-performance binary Zsh module that enhances shell script execution through:

- **Automatic compilation** of Zsh scripts to optimized bytecode (.zwc files)
- **Comprehensive performance tracking** for sourced files with detailed timing metrics
- **Advanced path caching** and filesystem operation optimization
- **Seamless integration** with the [Zi plugin manager](https://github.com/z-shell/zi)
- **Cross-platform compatibility** across Linux, macOS, and various Unix systems

## Repository Structure

- `Src/`: Core C source files for the zpmod module
- `Config/`: Build configuration files and version information
- `Scripts/`: Utility scripts for building, installing, and maintenance
- `docs/`: Comprehensive documentation
- `Test/`: Test cases for the module

## Development Workflow

### Building the Module

```bash
# Configure the build
./configure

# Build the module
make

# Install the module
make install
```

### Testing

```bash
# Run the test suite
make test
```

### Key Scripts

- `Scripts/install.sh`: Main installation script
- `Scripts/clean.sh`: Cleans build artifacts and temporary files
- `Scripts/update-readme.sh`: Updates README.md based on documentation

## Code Architecture

The module follows Zsh's module architecture with these key components:

1. **Module Entry Point**: `Src/module.c` defines initialization and cleanup functions
2. **Core Functionality**:
   - `Src/exec.c`: Handles script compilation and execution
   - `Src/hashtable.c`: Manages hash tables for performance
   - `Src/utils.c`: Utility functions used throughout the module

3. **Integration Points**:
   - Hook into Zsh's source command to track performance
   - File path caching system to reduce filesystem operations
   - Signal handling for clean shutdowns

## Project Conventions

### Build System

- Uses autoconf/automake for configuration
- Config files in `Config/` directory
- Files ending in `.pro` are prototype declarations
- Files ending in `.epro` are exported prototype declarations

### Documentation Strategy

- Documentation-driven development approach
- `docs/` directory contains detailed documentation
- Root `README.md` is automatically generated from docs using `Scripts/update-readme.sh`

### Temporary Files

- Build process creates temporary `.mdh.tmp` files that are automatically cleaned
- Run `Scripts/clean.sh` to remove all temporary files and build artifacts
- The `.gitignore` file lists patterns for temporary files that should not be committed

## Critical Details

1. **File Descriptor Handling**: The module carefully manages file descriptors to prevent leaks. Always check FD validity before operations.

2. **Memory Management**: Uses Zsh's memory allocation functions (`zalloc`, `zfree`) rather than standard malloc/free.

3. **Error Handling**:
   - Returns meaningful error codes
   - Uses `zwarnnam()` for warnings
   - Uses `zerrnam()` for errors

4. **Cross-Platform Compatibility**:
   - Tested on Linux, macOS, and various Unix systems
   - Contains platform-specific code paths (see `#ifdef` sections)

## Example Patterns

### Adding New Features

```c
// Example of adding a new module feature
static int
bin_zpmod_new_feature(char *name, char **args, Options ops, UNUSED(int func))
{
    // Feature implementation
    return 0;
}
```

### Error Handling Pattern

```c
if (fd < 0) {
    zwarnnam(name, "can't open file: %e", errno);
    return 1;
}
```

## Common Pitfalls

1. File descriptor exhaustion - always close opened file descriptors
2. Signal handling issues - use Zsh's signal handling mechanisms
3. Memory leaks - use `zalloc`/`zfree` consistently
4. Incorrect error propagation - ensure error codes are properly returned
5. Compatibility issues - test on all supported platforms

## Contribution Guidelines

1. **Code Style**: Follow the existing code style and conventions. Use `clang-format` for formatting C code.
2. **Commit Messages**: Write clear and descriptive commit messages. Use the imperative mood ("Add feature" not "Added feature").
3. **Testing**: Include tests for new features and bug fixes. Run the test suite before submitting changes.
4. **Documentation**: Update documentation to reflect changes. Use `Scripts/update-readme.sh` to regenerate README.md.
5. **Pull Requests**: Submit changes via pull requests. Include a description of the changes and any relevant issue numbers.
6. **Consistency of Organization**: Ensure consistent organization and structure across [all repositories](https://github.com/orgs/z-shell/repositories).

## Best Practices

- Use Zsh's built-in functions for file operations to ensure compatibility
- Avoid using global variables; prefer passing data through function parameters
- Keep functions small and focused on a single task
- Use meaningful variable and function names to improve readability
- Regularly review and refactor code to maintain quality and performance

## Additional Resources

- [Zsh Module Documentation](https://zsh.sourceforge.io/Doc/Release/Modules.html)
- [Zsh Developer Guide](https://zsh.sourceforge.io/Doc/Release/Developer-Guide.html)
- [Zi Plugin Manager](https://github.com/z-shell/zi)
  - [Zi Plugin Manager Documentation](https://wiki.zshell.dev)
- [Zsh Performance Tips](https://zsh.sourceforge.io/Doc/Release/Performance.html)
- [Z-Shell Organization](https://github.com/z-shell)
  - [Z-Shell Repositories](https://github.com/orgs/z-shell/repositories)

## Code Architecture

The module follows Zsh's module architecture with these key components:

1. **Module Entry Point**: `Src/module.c` defines initialization and cleanup functions
2. **Core Functionality**:
   - `Src/exec.c`: Handles script compilation and execution
   - `Src/hashtable.c`: Manages hash tables for performance
   - `Src/utils.c`: Utility functions used throughout the module

3. **Integration Points**:
   - Hook into Zsh's source command to track performance
   - File path caching system to reduce filesystem operations
   - Signal handling for clean shutdowns

## Project Conventions

### Build System

- Uses autoconf/automake for configuration
- Config files in `Config/` directory
- Files ending in `.pro` are prototype declarations
- Files ending in `.epro` are exported prototype declarations

### Documentation Strategy

- Documentation-driven development approach
- `docs/` directory contains detailed documentation
- Root `README.md` is automatically generated from docs using `Scripts/update-readme.sh`

### Temporary Files

- Build process creates temporary `.mdh.tmp` files that are automatically cleaned
- Run `Scripts/clean.sh` to remove all temporary files and build artifacts
- The `.gitignore` file lists patterns for temporary files that should not be committed

## Critical Details

1. **File Descriptor Handling**: The module carefully manages file descriptors to prevent leaks. Always check FD validity before operations.

2. **Memory Management**: Uses Zsh's memory allocation functions (`zalloc`, `zfree`) rather than standard malloc/free.

3. **Error Handling**:
   - Returns meaningful error codes
   - Uses `zwarnnam()` for warnings
   - Uses `zerrnam()` for errors

4. **Cross-Platform Compatibility**:
   - Tested on Linux, macOS, and various Unix systems
   - Contains platform-specific code paths (see `#ifdef` sections)

## Example Patterns

### Adding New Features

```c
// Example of adding a new module feature
static int
bin_zpmod_new_feature(char *name, char **args, Options ops, UNUSED(int func))
{
    // Feature implementation
    return 0;
}
```

### Error Handling Pattern

```c
if (fd < 0) {
    zwarnnam(name, "can't open file: %e", errno);
    return 1;
}
```

## Common Pitfalls

1. File descriptor exhaustion - always close opened file descriptors
2. Signal handling issues - use Zsh's signal handling mechanisms
3. Memory leaks - use `zalloc`/`zfree` consistently
4. Incorrect error propagation - ensure error codes are properly returned
5. Compatibility issues - test on all supported platforms

## Contribution Guidelines

1. **Code Style**: Follow the existing code style and conventions. Use `clang-format` for formatting C code.
2. **Commit Messages**: Write clear and descriptive commit messages. Use the imperative mood ("Add feature" not "Added feature").
3. **Testing**: Include tests for new features and bug fixes. Run the test suite before submitting changes.
4. **Documentation**: Update documentation to reflect changes. Use `Scripts/update-readme.sh` to regenerate README.md.
5. **Pull Requests**: Submit changes via pull requests. Include a description of the changes and any relevant issue numbers.
6. **Consistency of Organization**: Ensure consistent organization and structure across [all repositories](https://github.com/orgs/z-shell/repositories).

## Best Practices

- Use Zsh's built-in functions for file operations to ensure compatibility
- Avoid using global variables; prefer passing data through function parameters
- Keep functions small and focused on a single task
- Use meaningful variable and function names to improve readability
- Regularly review and refactor code to maintain quality and performance

## Additional Resources

- [Zsh Module Documentation](https://zsh.sourceforge.io/Doc/Release/Modules.html)
- [Zsh Developer Guide](https://zsh.sourceforge.io/Doc/Release/Developer-Guide.html)
- [Zi Plugin Manager](https://github.com/z-shell/zi)
  - [Zi Plugin Manager Documentation](https://wiki.zshell.dev)
- [Zsh Performance Tips](https://zsh.sourceforge.io/Doc/Release/Performance.html)
- [Z-Shell Organization](https://github.com/z-shell)
  - [Z-Shell Repositories](https://github.com/orgs/z-shell/repositories)
