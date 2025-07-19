# Documentation Workflow

This document outlines the documentation workflow and tools we've set up to maintain consistency across the repository.

## Overview

We've implemented a documentation-driven approach with the following components:

1. **Documentation Directory (`docs/`)**: Contains comprehensive documentation files
2. **Root README.md**: Provides a high-level overview with links to detailed documentation
3. **Automatic Sync Tool**: Keeps the README.md in sync with documentation changes
4. **GitHub Actions Workflow**: Automatically updates README.md when documentation changes

## Components

### Documentation Structure

- **Root README.md**: Entry point for GitHub repository visitors
  - Contains: Project overview, key features, quick install, links to docs
  - Purpose: Quick introduction to the project

- **docs/ Directory**: Comprehensive documentation
  - `index.md`: Main documentation entry point
  - `GUIDE.md`: User installation and usage instructions
  - `API.md`: Technical API reference
  - `IMPROVEMENTS.md`: Recent and planned technical improvements
  - `CONTRIBUTING.md`: Guidelines for contributors

### Automation Tools

- **update-readme.sh Script**:
  - Location: `./Scripts/update-readme.sh`
  - Purpose: Updates README.md based on documentation in the docs/ directory
  - Usage:
    - `./Scripts/update-readme.sh` - Update README.md
    - `./Scripts/update-readme.sh --check-only` - Check if update is needed
    - `./Scripts/update-readme.sh --verbose` - Verbose output during update

- **GitHub Actions Workflow**:
  - Location: `.github/workflows/sync-docs.yml`
  - Purpose: Automatically runs update-readme.sh when documentation changes
  - Triggers: On push to main/master branch that changes files in docs/

- **Pull Request Template**:
  - Location: `.github/PULL_REQUEST_TEMPLATE/pull_request_template.md`
  - Purpose: Reminds contributors to keep documentation in sync

## Workflow for Contributors

When making changes to the repository:

1. **Update Documentation**: Make necessary changes to files in the docs/ directory
2. **Sync README.md**: Run `./Scripts/update-readme.sh` to update the README.md
3. **Verify Sync**: Check that README.md is correctly updated and links work
4. **Submit Changes**: Create a pull request with both documentation and code changes

This approach ensures that documentation remains up-to-date and consistent across the repository.
