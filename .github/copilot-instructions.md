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
- `Scripts/advanced-install.sh`: Advanced installation with additional options

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

This repository follows the **Divio Documentation System** for consistent, maintainable documentation across the Z-Shell organization. All repositories should implement this structure.

#### Divio Documentation System Structure

The `docs/` directory is organized into four distinct categories:

```text
docs/
├── tutorials/          # Learning-oriented (hands-on lessons)
├── how-to/             # Problem-oriented (practical guides)
├── reference/          # Information-oriented (technical specs)
├── explanation/        # Understanding-oriented (background knowledge)
├── index.md            # Main documentation hub
└── CONTRIBUTING.md     # Contribution guidelines (root level for GitHub visibility)
```

#### Documentation Categories

1. **`tutorials/`** - Learning-oriented documentation
   - Step-by-step guides for beginners
   - Complete meaningful projects from start to finish
   - Focus on building confidence through successful completion
   - Example: `getting-started.md` with complete installation and first usage

2. **`how-to/`** - Problem-oriented documentation
   - Solutions to specific problems
   - Assume some knowledge and focus on getting things done
   - Task-oriented with clear outcomes
   - Examples: `configure-lazy-loading.md`, `optimize-compilation.md`

3. **`reference/`** - Information-oriented documentation
   - Technical specifications, API documentation
   - Comprehensive details organized for lookup
   - Dry, factual information
   - Examples: `api.md`, command references, configuration options

4. **`explanation/`** - Understanding-oriented documentation
   - Background knowledge and architectural discussions
   - Explains why things work the way they do
   - Provides context and deeper understanding
   - Examples: `internal-architecture.md`, `technical-improvements.md`

#### Documentation Guidelines

**When adding new documentation:**

1. **Determine the category** - Ask: "Is this teaching, solving a problem, providing reference info, or explaining concepts?"
2. **Place in correct directory** - Use the appropriate category folder
3. **Follow naming conventions** - Use descriptive, kebab-case filenames
4. **Update category README** - Add entry to the relevant `README.md` file
5. **Link from index.md** - Ensure discoverability from main documentation page

**When editing existing documentation:**

1. **Maintain category integrity** - Don't mix tutorial content in reference docs
2. **Update cross-references** - Check and update any links that may be affected
3. **Follow the category's writing style** - Tutorials are hands-on, references are factual, etc.

**Consistency across Z-Shell organization:**

- All repositories should implement this same structure
- Use identical category names and README formats
- Maintain consistent navigation and cross-linking patterns
- Apply this structure when creating new repositories or refactoring existing documentation

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

## Organization-Level GitHub Actions

The Z-Shell organization maintains a comprehensive set of reusable GitHub Actions at [z-shell/.github/actions/](https://github.com/z-shell/.github/actions/) to ensure consistency and reduce code duplication across repositories:

### Available Actions

1. **setup-zsh** - Sets up Zsh environment and dependencies
2. **setup-zsh-development** - Complete development environment setup including build tools and dependencies
3. **build-zpmod-module** - Automated building of zpmod module with proper configuration and verification
4. **test-zsh-module** - Loads and tests Zsh modules with comprehensive functionality testing
5. **test-zpmod-module** - Comprehensive testing of zpmod module functionality with customizable test files
6. **determine-branch** - Determines the correct branch name for PR vs push events
7. **mirror** - SSH-based repository mirroring for synchronization
8. **rclone** - Cloud storage synchronization using rclone
9. **rebase** - Automated PR rebasing via comment triggers
10. **commit** - Automated git commits for CI/CD workflows

### Zsh Module Development Workflow

The organization provides a complete Zsh module development workflow:

1. **setup-zsh-development** - Sets up development environment with build tools (autoconf, automake, build-essential)
2. **build-zpmod-module** - Zpmod-specific building with proper verification and error handling (uses zpmod's custom install script)
3. **test-zsh-module** - Generic Zsh module testing with comprehensive functionality testing
4. **test-zpmod-module** - Zpmod-specific testing with customizable test files and detailed verification
5. **determine-branch** - Branch determination utility for PR vs push event workflows

These actions significantly enhance the development workflow for zpmod and other Zsh modules in the organization.

### Integration Guidelines

**Before implementing custom workflow steps:**

1. Check [z-shell/.github/actions/](https://github.com/z-shell/.github/actions/) for existing organization actions
2. Use organization actions instead of custom implementations when available
3. Follow the examples in each action's README for proper usage
4. Contribute back to organization actions if custom functionality would benefit other repositories

**Available Zsh-specific actions:**

- `setup-zsh-development` for complete development environment setup
- `build-zpmod-module` for zpmod-specific building with verification (uses custom install script)
- `test-zsh-module` for comprehensive module testing and validation
- `test-zpmod-module` for zpmod-specific testing with customizable test files
- `determine-branch` for branch determination in PR vs push workflows

**Current usage in this repository:**

- Fully optimized to use organization actions throughout the CI/CD pipeline
- Uses `setup-zsh-development` for complete environment setup (replacing manual build dependencies)
- Uses `determine-branch` for clean branch detection logic
- Uses `build-zpmod-module` for standardized, verified zpmod building
- Uses `test-zpmod-module` for comprehensive zpmod functionality testing
- **Result**: Reduced workflow complexity by 61% while improving maintainability and reusability

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
4. **Documentation**: Update documentation to reflect changes. Follow the Divio documentation system structure in `docs/`. Place new documentation in the appropriate category (tutorials/, how-to/, reference/, explanation/) and update the relevant README.md files.
5. **Pull Requests**: Submit changes via pull requests. Include a description of the changes and any relevant issue numbers.
6. **GitHub Actions**: Before adding custom workflow steps, check [z-shell/.github/actions/](https://github.com/z-shell/.github/actions/) for existing organization-level actions that provide the same functionality.
7. **Consistency of Organization**: Ensure consistent organization and structure across [all repositories](https://github.com/orgs/z-shell/repositories).

## Best Practices

### Code Quality

- Use Zsh's built-in functions for file operations to ensure compatibility
- Avoid using global variables; prefer passing data through function parameters
- Keep functions small and focused on a single task
- Use meaningful variable and function names to improve readability
- Regularly review and refactor code to maintain quality and performance

### Documentation Maintenance

- **Always follow the Divio documentation system** when adding or modifying documentation
- **Categorize correctly**: Ask yourself whether content is teaching (tutorials), problem-solving (how-to), informational (reference), or explanatory (explanation)
- **Update navigation**: When adding new documentation, update the appropriate category README.md and link from `docs/index.md`
- **Maintain consistency**: Use the same structure and naming conventions across all Z-Shell organization repositories
- **Cross-reference properly**: Ensure internal links are updated when moving or renaming documentation files

## Additional Resources

- [Zsh Module Documentation](https://zsh.sourceforge.io/Doc/Release/Modules.html)
- [Zsh Developer Guide](https://zsh.sourceforge.io/Doc/Release/Developer-Guide.html)
- [Zi Plugin Manager](https://github.com/z-shell/zi)
  - [Zi Plugin Manager Documentation](https://wiki.zshell.dev)
- [Zsh Performance Tips](https://zsh.sourceforge.io/Doc/Release/Performance.html)
- [Z-Shell Organization](https://github.com/z-shell)
  - [Z-Shell Repositories](https://github.com/orgs/z-shell/repositories)
