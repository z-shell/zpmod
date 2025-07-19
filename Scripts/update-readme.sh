#!/usr/bin/env bash
# =============================================================================
# README.md Sync Script
# =============================================================================
#
# This script ensures the root README.md is kept in sync with the documentation.
# It extracts key information from the docs directory and updates the README.md.
#
# Usage: ./Scripts/update-readme.sh [OPTIONS]
# =============================================================================

set -euo pipefail

# Configuration
readonly SCRIPT_NAME="$(basename "$0")"
readonly ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
readonly DOCS_DIR="${ROOT_DIR}/docs"
readonly README_PATH="${ROOT_DIR}/README.md"

# Colors for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# Options
VERBOSE=false
CHECK_ONLY=false

# =============================================================================
# Utility Functions
# =============================================================================

log() {
  local level="$1"
  shift

  case "$level" in
  "INFO") echo -e "${BLUE}[INFO]${NC}  $*" >&2 ;;
  "WARN") echo -e "${YELLOW}[WARN]${NC}  $*" >&2 ;;
  "ERROR") echo -e "${RED}[ERROR]${NC} $*" >&2 ;;
  "SUCCESS") echo -e "${GREEN}[SUCCESS]${NC} $*" >&2 ;;
  "DEBUG") [[ $VERBOSE == true ]] && echo -e "${BLUE}[DEBUG]${NC} $*" >&2 ;;
  esac
}

show_help() {
  cat <<EOF
Usage: ${SCRIPT_NAME} [OPTIONS]

This script updates the root README.md file from documentation in the docs directory.

Options:
  -h, --help         Show this help message
  -v, --verbose      Enable verbose output
  -c, --check-only   Check if README.md needs updating without modifying it

Examples:
  ${SCRIPT_NAME}              # Update README.md
  ${SCRIPT_NAME} --check-only # Just check if update is needed
EOF
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
    -h | --help)
      show_help
      exit 0
      ;;
    -v | --verbose)
      VERBOSE=true
      shift
      ;;
    -c | --check-only)
      CHECK_ONLY=true
      shift
      ;;
    *)
      log "ERROR" "Unknown option: $1"
      show_help
      exit 1
      ;;
    esac
  done
}

# =============================================================================
# Update Functions
# =============================================================================

# Check if required files exist
check_files() {
  log "INFO" "Checking for required files..."

  local missing_files=false

  if [[ ! -d $DOCS_DIR ]]; then
    log "ERROR" "Docs directory not found: $DOCS_DIR"
    missing_files=true
  fi

  for file in GUIDE.md API.md IMPROVEMENTS.md CONTRIBUTING.md; do
    if [[ ! -f "${DOCS_DIR}/${file}" ]]; then
      log "ERROR" "Required documentation file not found: ${DOCS_DIR}/${file}"
      missing_files=true
    fi
  done

  if [[ $missing_files == true ]]; then
    return 1
  else
    log "SUCCESS" "All required files found"
    return 0
  fi
}

# Extract key features from documentation
extract_key_features() {
  log "DEBUG" "Extracting key features from documentation..."

  # Try to extract from GUIDE.md first
  local key_features=$(sed -n '/## Features/,/^## /p' "${DOCS_DIR}/GUIDE.md" 2>/dev/null | grep "^- " | head -n 4)

  # If not found in GUIDE.md, try index.md
  if [[ -z $key_features ]]; then
    key_features=$(sed -n '/## Features/,/^## /p' "${DOCS_DIR}/index.md" 2>/dev/null | grep "^- " | head -n 4)
  fi

  # If still not found, use existing features from README.md
  if [[ -z $key_features && -f $README_PATH ]]; then
    key_features=$(sed -n '/## 🚀 Key Features/,/^## /p' "$README_PATH" | grep "^- " | head -n 4)
  fi

  # If still not found, use default features
  if [[ -z $key_features ]]; then
    key_features='- **Intelligent Script Compilation**: Automatically compiles `.zsh` scripts to optimized `.zwc` bytecode
- **Advanced Performance Tracking**: Comprehensive timing analysis for all sourced files
- **Robust Error Handling**: Graceful handling of edge cases including file descriptors and device files
- **Seamless Zi Integration**: Enhanced performance tracking with the Zi plugin manager'
  fi

  echo "$key_features"
}

# Generate the README.md content
generate_readme() {
  log "INFO" "Generating README.md content..."

  local key_features=$(extract_key_features)

  cat <<EOF
# Module: \`zpmod\`

<div align="center">

[![🍎 Build (MacOS)](https://github.com/z-shell/zpmod/actions/workflows/test-macos.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/test-macos.yml)
[![🐧 Build (Linux)](https://github.com/z-shell/zpmod/actions/workflows/test-linux.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/test-linux.yml)
[![📦 Create Release](https://github.com/z-shell/zpmod/actions/workflows/release.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/release.yml)

</div><hr />

\`zpmod\` is a high-performance binary Zsh module that revolutionizes shell script execution through intelligent automatic compilation and comprehensive performance tracking.

## 🚀 Key Features

$key_features

## 📦 Installation

For detailed installation instructions, please refer to:

- [Installation with Zi](docs/GUIDE.md#installation-with-zi) - Recommended method
- [Manual Installation](docs/GUIDE.md#manual-installation) - Step-by-step guide
- [Pre-built Binaries](docs/GUIDE.md#pre-built-binaries) - Quick download options

## 📚 Documentation

For comprehensive documentation, please visit our [documentation pages](docs/index.md):

- [User Guide](docs/GUIDE.md) - Detailed installation and usage instructions
- [API Reference](docs/API.md) - Technical reference and command details
- [Technical Improvements](docs/IMPROVEMENTS.md) - Recent and planned enhancements
- [Contributing Guide](docs/CONTRIBUTING.md) - How to contribute to the project

## 📄 License

The zpmod module is available under the same license as Zsh itself. See the [LICENSE](LICENSE) file for details.
EOF
}

# Update the README.md file
update_readme() {
  log "INFO" "Updating README.md..."

  local temp_file="${README_PATH}.new"
  generate_readme >"$temp_file"

  # Check if there are actual differences
  if diff -q "$temp_file" "$README_PATH" >/dev/null 2>&1; then
    log "SUCCESS" "README.md is already up to date"
    rm "$temp_file"
    return 0
  else
    if [[ $CHECK_ONLY == true ]]; then
      log "WARN" "README.md needs to be updated"
      rm "$temp_file"
      return 1
    else
      mv "$temp_file" "$README_PATH"
      log "SUCCESS" "README.md has been updated"
      return 0
    fi
  fi
}

# =============================================================================
# Main Function
# =============================================================================

main() {
  log "INFO" "Starting README.md update process..."

  if ! check_files; then
    log "ERROR" "Required files missing, cannot update README.md"
    exit 1
  fi

  if ! update_readme; then
    if [[ $CHECK_ONLY == true ]]; then
      log "WARN" "README.md needs to be updated"
      exit 1
    else
      log "ERROR" "Failed to update README.md"
      exit 1
    fi
  fi

  log "SUCCESS" "README.md update process complete"
}

# =============================================================================
# Script Execution
# =============================================================================

parse_args "$@"
main
