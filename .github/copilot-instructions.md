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

## AI Agent Instructions

### Code Quality Workflow for AI Agents

After modifying code, always run trunk checks to ensure code quality:

```bash
# Run all code quality checks (excludes network-dependent linters)
trunk check -y --filter=-trufflehog,-semgrep

# Run zpmod-specific maintenance checks
trunk check --filter=zpmod-maintenance

# For faster feedback during development
trunk check --filter=zpmod-maintenance --sample=10

# Format code only
trunk fmt
```

### AI Agent Development Workflow

1. **Start**: `./Scripts/maintenance.sh check-health`
2. **Develop**: Make changes, run `trunk check --filter=zpmod-maintenance --sample=10`
3. **Validate**: `./Scripts/maintenance.sh lint-code`
4. **Commit**: `./Scripts/maintenance.sh comprehensive`

### AI Agent Error Handling

- Review trunk output for actionable feedback
- Use `VERBOSE=1` with maintenance commands for debugging
- Check maintenance logs for detailed error information
- Validate fixes by re-running checks

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
- `Scripts/maintenance.sh`: Comprehensive workspace maintenance and cleaning
- `Scripts/advanced-install.sh`: Advanced installation with additional options

## Workspace Maintenance & Code Quality

### Maintenance System

The repository uses a centralized maintenance system through `Scripts/maintenance.sh` that provides comprehensive workspace management:

#### Core Functions

```bash
# Health checks and version validation
./Scripts/maintenance.sh check-health
./Scripts/maintenance.sh check-versions

# Code quality and security
./Scripts/maintenance.sh lint-code
./Scripts/maintenance.sh security-scan

# Build artifact and temporary file cleanup
./Scripts/maintenance.sh clean-deep

# Configuration validation
./Scripts/maintenance.sh validate-config

# Complete maintenance workflow
./Scripts/maintenance.sh comprehensive
```

#### Environment Variables

- `VERBOSE=1`: Enable detailed output for cleaning operations and debugging

### Trunk.io Integration

The project integrates with [trunk.io](https://trunk.io) for advanced code quality management through a custom linter system.

#### Configuration

- **File**: `.trunk/trunk.yaml` - Main trunk configuration
- **Actions**: `.trunk/actions/zpmod-maintenance/` - Custom Python actions for maintenance integration
- **Custom Linter**: `zpmod-maintenance` - Organization-specific quality checks
- **AI Agent Instructions**: See "AI Agent Instructions" section above for GitHub Copilot and other AI coding assistants

#### Available Commands

```bash
# Run all quality checks
trunk check

# Run specific maintenance checks
trunk check --filter=zpmod-maintenance

# Sample a subset of files for faster feedback
trunk check --filter=zpmod-maintenance --sample=10

# AI agent recommended workflow (excludes network-dependent linters)
trunk check -y --filter=-trufflehog,-semgrep
```

#### Custom Linter Features

The `zpmod-maintenance` linter provides three specialized commands:

1. **health-check**: Version consistency and workspace validation
2. **version-check**: Comprehensive version string verification across files
3. **clean**: Deep workspace cleaning with detailed progress tracking

#### Integration Benefits

- **Consistency**: Standardized quality checks across the entire Z-Shell organization
- **Automation**: Seamless CI/CD integration with quality gates
- **Extensibility**: Custom actions framework for organization-specific requirements
- **Performance**: Efficient file sampling and targeted checking
- **Developer Experience**: Clear feedback with actionable error reporting

### Future Improvements

The trunk implementation has significant potential for organization-wide enhancement:

- **Cross-Repository Standards**: Shared linter configurations across Z-Shell projects
- **Advanced Caching**: Workspace-aware dependency caching for faster builds
- **Smart Filtering**: Context-aware file selection based on project structure
- **Custom Actions Library**: Reusable maintenance actions for common Z-Shell patterns
- **Performance Metrics**: Build and maintenance time tracking across projects

#### Organization-Wide Implementation Roadmap

##### Phase 1: Template Standardization

- Create `.trunk/` template configurations for all Z-Shell repositories
- Develop shared custom linters for common Z-Shell patterns (zsh modules, documentation, shell scripts)
- Implement organization-level trunk configuration inheritance

##### Phase 2: Enhanced Automation

- Build cross-repository quality metrics dashboard
- Implement automated dependency checking across Z-Shell ecosystem
- Create shared CI/CD templates with trunk integration

##### Phase 3: Advanced Features

- Develop intelligent file change detection for faster trunk runs
- Implement workspace-aware caching for multi-repository development
- Create custom trunk plugins for Zsh-specific analysis (performance profiling, module compatibility)

##### Phase 4: Developer Experience

- Build VS Code/IDE extensions for seamless trunk integration
- Implement real-time quality feedback during development
- Create automated contribution workflow with trunk quality gates

##### Implementation Benefits for Z-Shell Organization

- **Consistency**: Uniform code quality standards across all 50+ repositories
- **Efficiency**: Reduced CI/CD time through intelligent caching and filtering
- **Quality**: Automated detection of organization-specific issues and patterns
- **Scalability**: Template-based approach for easy addition of new repositories
- **Collaboration**: Shared quality tools reduce learning curve for contributors

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
- Run `Scripts/maintenance.sh clean-deep` to remove all temporary files and build artifacts
- The `.gitignore` file lists patterns for temporary files that should not be committed

### Documentation File Placement

**CRITICAL**: All documentation must follow the Divio documentation system structure:

- **✅ CORRECT**: Place documentation in `docs/` subdirectories (`tutorials/`, `how-to/`, `reference/`, `explanation/`)
- **❌ FORBIDDEN**: Never create documentation files in the workspace root (e.g., `SECURITY-FIXES.md`, `CHANGES.md`)
- **Exception**: Only `README.md` and `CONTRIBUTING.md` are allowed in the root for GitHub visibility
- **Always**: Update `docs/index.md` to link new documentation and maintain proper navigation

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
8. **Workspace Maintenance**: Use the integrated maintenance system for all cleaning and quality checks:

### Pre-Contribution Workflow

```bash
# Before starting development
./Scripts/maintenance.sh check-health

# During development
./Scripts/maintenance.sh lint-code

# Before committing
./Scripts/maintenance.sh comprehensive
```

### Trunk Integration Workflow

```bash
# Quick quality check
trunk check --filter=zpmod-maintenance --sample=10

# Full repository scan
trunk check --filter=zpmod-maintenance

# Individual maintenance commands via trunk
trunk check --filter=zpmod-maintenance  # Runs all: health-check, version-check, clean

# AI agent recommended workflow (excludes network-dependent linters)
trunk check -y --filter=-trufflehog,-semgrep
```

### Quality Standards

- **Always run** `./Scripts/maintenance.sh comprehensive` before submitting PRs
- **Use trunk integration** for consistent code quality across the organization
- **Clean workspace** with `VERBOSE=1 ./Scripts/maintenance.sh clean-deep` when troubleshooting
- **Validate versions** with `./Scripts/maintenance.sh check-versions` after version updates

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
