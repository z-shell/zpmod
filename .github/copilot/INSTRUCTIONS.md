# GitHub Copilot Repository Instructions

## Repository Structure

This repository follows standard GitHub best practices:

1. **Root Directory**: Contains essential module files and the primary README.md
2. **Documentation**: Comprehensive documentation in the `/docs/` directory
3. **GitHub Configuration**: GitHub-specific files in the `/.github/` directory
4. **Module Code**: Source code in appropriate directories (`Src/`, `Config/`, etc.)

### File Organization Rules

1. **Module Code Location**:
   - All module-related code should be in the root directories (`Config`, `Scripts`, `Src`, `Test`, `Util`)
   - The `Src/zi/` directory contains the core module implementation
   - **All scripts** (including utility scripts, maintenance scripts, etc.) should be in the `Scripts/` directory

2. **Documentation Location**:
   - User-facing documentation should be in the `/docs/` directory
   - `README.md` in the root is the primary documentation entry point
   - Technical documentation should be in the `/docs/` directory

3. **Path Handling**:
   - In root `README.md`: Use paths like `docs/GUIDE.md` or `Scripts/install.sh`
   - For links in documentation, ensure they point to the correct relative locations

## When Making Changes

1. **For Documentation Changes**:
   - Update documents in the `/docs/` directory
   - Keep the root `README.md` as a high-level overview with links to detailed docs
   - Follow the existing markdown style for consistency

2. **For Module Code Changes**:
   - Place all code in the appropriate root directories
   - Follow Zsh module development conventions
   - Use the existing build system (autoconf/automake)

3. **For Version Updates**:
   - Update version numbers in both documentation and code
   - Update `Config/version.mk` for all releases

## Directory Structure Reference

**Root Structure**:

```
/
├── Config/             # Configuration files and templates
├── Scripts/            # Shell scripts for building, installing, and utility functions
├── Src/                # Source code for the module
│   └── zi/             # Module implementation directory
├── Test/               # Test suite for the module
├── Util/               # Utility scripts and tools
├── docs/               # Comprehensive documentation
├── README.md           # Primary documentation entry point
├── LICENSE             # License file
├── configure.ac        # Autoconf configuration
├── Makefile.in         # Makefile template
└── ...                 # Other build-related files
```

**Documentation Structure**:

```
/docs/
├── API.md              # API reference documentation
├── CONTRIBUTING.md     # Contribution guidelines
├── GUIDE.md            # User guide
├── IMPROVEMENTS.md     # Technical improvements documentation
└── index.md            # Documentation index
```

**GitHub Structure**:

```
/.github/
├── workflows/          # GitHub Actions workflows
├── copilot/            # GitHub Copilot instructions
│   ├── INSTRUCTIONS.md # This file
│   └── REQUIREMENTS.md # Project requirements
├── ISSUE_TEMPLATE/     # Issue templates
└── PULL_REQUEST_TEMPLATE.md # PR template
```

**Important Note**:

- The `.github/` directory should only contain GitHub-specific files and configuration
- User-facing documentation should be in the `/docs/` directory
- Only essential files should be in the repository root
- All scripts should be in the root `Scripts/` directory
- All configuration templates should be in the root `Config/` directory
