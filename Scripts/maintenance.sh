#!/usr/bin/env bash
# maintenance.sh - Workspace maintenance utilities for zpmod
# Usage: ./Scripts/maintenance.sh [command]
#
# This script provides comprehensive workspace maintenance capabilities
# and integrates with trunk.io code quality tools for automated checks.

set -euo pipefail

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Exit codes for trunk integration
readonly EXIT_SUCCESS=0
readonly EXIT_FAILURE=1
readonly EXIT_WARNING=2

log() {
  local level="$1"
  shift
  case "${level}" in
  ERROR) echo -e "${RED}[ERROR]${NC} $*" >&2 ;;
  SUCCESS) echo -e "${GREEN}[SUCCESS]${NC} $*" ;;
  WARNING) echo -e "${YELLOW}[WARNING]${NC} $*" ;;
  INFO) echo -e "${BLUE}[INFO]${NC} $*" ;;
  *) echo "$*" ;;
  esac
}

show_usage() {
  cat <<'EOF'
zpmod Maintenance Utilities

Usage: ./Scripts/maintenance.sh [command]

Commands:
  check-health       Check overall workspace health
  lint-code         Run code quality checks
  update-docs       Update documentation cross-references
  check-versions    Verify version consistency
  clean-build       Clean build artifacts and temporary files (basic)
  clean-deep        Deep clean all artifacts including generated files
  validate-config   Validate configuration files
  security-scan     Basic security checks
  comprehensive     Run all maintenance checks (for trunk integration)
  comprehensive-with-clean  Run comprehensive checks with deep clean first

  help              Show this help message

Examples:
  ./Scripts/maintenance.sh check-health
  ./Scripts/maintenance.sh lint-code
  ./Scripts/maintenance.sh clean-deep      # Complete workspace cleanup
  ./Scripts/maintenance.sh comprehensive

Environment Variables:
  VERBOSE=1         Enable verbose output for cleaning operations

Trunk Integration:
  This script integrates with trunk.io code quality tools.
  Run 'trunk check --filter=zpmod-maintenance' to execute maintenance checks.
EOF
}

check_health() {
  log "INFO" "Running workspace health check..."

  local issues=0

  # Check required files
  local required_files=(
    "Config/zpmod-version.mk"
    "Src/zi/zpmod.c"
    "Scripts/bump-version.sh"
    "docs/index.md"
    "CHANGELOG.md"
  )

  for file in "${required_files[@]}"; do
    if [[ ! -f "${PROJECT_ROOT}/${file}" ]]; then
      log "ERROR" "Missing required file: ${file}"
      ((issues++))
    fi
  done

  # Check Divio documentation structure
  local doc_dirs=("tutorials" "how-to" "reference" "explanation")
  for dir in "${doc_dirs[@]}"; do
    if [[ ! -d "${PROJECT_ROOT}/docs/${dir}" ]]; then
      log "ERROR" "Missing documentation directory: docs/${dir}"
      ((issues++))
    elif [[ ! -f "${PROJECT_ROOT}/docs/${dir}/README.md" ]]; then
      log "WARNING" "Missing README.md in docs/${dir}"
      ((issues++))
    fi
  done

  # Check version consistency
  local version_check_result=0
  check_versions
  version_check_result=$?
  if [[ ${version_check_result} -eq 1 ]]; then
    ((issues++))
  fi

  if [[ ${issues} -eq 0 ]]; then
    log "SUCCESS" "Workspace health check passed ✓"
    return 0
  else
    log "ERROR" "Found ${issues} issues"
    return 1
  fi
}

check_versions() {
  log "INFO" "Checking version consistency..."

  local mk_file="${PROJECT_ROOT}/Config/zpmod-version.mk"
  local c_file="${PROJECT_ROOT}/Src/zi/zpmod.c"

  if [[ ! -f ${mk_file} ]]; then
    log "ERROR" "Version file not found: ${mk_file}"
    return 1
  fi

  local mk_version
  mk_version=$(grep "^ZPMOD_VERSION=" "${mk_file}" | cut -d'=' -f2 | tr -d '"' | tr -d "'")

  if [[ -f ${c_file} ]]; then
    local c_version
    c_version=$(grep "^#define ZPMOD_VERSION " "${c_file}" | cut -d'"' -f2)

    if [[ ${mk_version} != "${c_version}" ]]; then
      log "ERROR" "Version mismatch:"
      log "ERROR" "  zpmod-version.mk: ${mk_version}"
      log "ERROR" "  zpmod.c: ${c_version}"
      log "INFO" "Run ./Scripts/bump-version.sh to sync versions"
      return 1
    fi
  fi

  log "SUCCESS" "Version consistency check passed (${mk_version})"
  return 0
}

lint_code() {
  log "INFO" "Running code quality checks..."

  # Check shell scripts
  if command -v shellcheck >/dev/null 2>&1; then
    log "INFO" "Running ShellCheck on shell scripts..."
    find "${PROJECT_ROOT}/Scripts" -name "*.sh" -exec shellcheck {} \;
  else
    log "WARNING" "shellcheck not found, skipping shell script linting"
  fi

  # Check for common issues in C code
  log "INFO" "Checking C code patterns..."
  cd "${PROJECT_ROOT}"

  # Check for unsafe string functions
  if grep -r "strcpy\|strcat\|sprintf\|gets" --include="*.c" --include="*.h" Src/ 2>/dev/null; then
    log "WARNING" "Found potentially unsafe string functions"
  fi

  # Check for TODO/FIXME comments
  if grep -r "TODO\|FIXME\|XXX\|HACK" --include="*.c" --include="*.h" --include="*.sh" Src/ Scripts/ 2>/dev/null; then
    log "INFO" "Found TODO/FIXME comments - consider creating issues"
  fi

  log "SUCCESS" "Code quality check completed"
}

update_docs() {
  log "INFO" "Updating documentation cross-references..."

  # This could be expanded to automatically update table of contents,
  # check for broken links, etc.

  log "INFO" "Documentation update completed"
}

clean_build() {
  log "INFO" "Cleaning build artifacts and temporary files..."

  cd "${PROJECT_ROOT}"

  # Clean standard build artifacts
  make clean 2>/dev/null || true

  # Clean backup files
  find . -name "*.backup" -delete 2>/dev/null || true
  find . -name "*.tmp" -delete 2>/dev/null || true
  find . -name ".mdh.tmp" -delete 2>/dev/null || true

  # Clean editor artifacts
  find . -name "*~" -delete 2>/dev/null || true
  find . -name ".#*" -delete 2>/dev/null || true

  log "SUCCESS" "Build cleanup completed"
}

clean_deep() {
  log "INFO" "Performing deep clean of all build artifacts and temporary files..."

  cd "${PROJECT_ROOT}"

  # Enable verbose mode if requested
  local verbose_mode=""
  if [[ ${VERBOSE-} == "1" ]]; then
    verbose_mode="-print"
    log "INFO" "Verbose mode enabled - showing files being removed"
  fi

  # Clean standard build artifacts
  log "INFO" "Removing compiled objects and libraries..."
  find . -type f \( -name "*.o" -o -name "*.so" -o -name "*.bundle" -o -name "*.a" -o -name "*.lo" -o -name "*.la" -o -name "*.dylib" \) "${verbose_mode}" -delete 2>/dev/null || true

  log "INFO" "Removing logs and cache files..."
  find . -type f \( -name "*.log" -o -name "*.stamp" -o -name "*.cache" -o -name "*.out" -o -name "*.pyc" -o -name "*.pyo" \) "${verbose_mode}" -delete 2>/dev/null || true

  log "INFO" "Removing editor backup files..."
  find . -type f \( -name "*~" -o -name "*.swp" -o -name "*.swo" \) "${verbose_mode}" -delete 2>/dev/null || true

  # Clean generated Makefiles (but preserve template files)
  log "INFO" "Removing generated Makefiles..."
  find . -name "Makefile" -not -path "./Makefile" -not -name "Makefile.in" "${verbose_mode}" -delete 2>/dev/null || true

  # Clean autoconf/automake files
  log "INFO" "Removing autoconf/automake artifacts..."
  rm -f config.log config.status config.h stamp-h || true

  # Clean generated code files
  log "INFO" "Removing generated source files..."
  find ./Src \( -name "*.mdh" -o -name "*.export" \) "${verbose_mode}" -delete 2>/dev/null || true
  find ./Src \( -name "*.pro" -o -name "*.epro" \) -not -name ".indent.pro" "${verbose_mode}" -delete 2>/dev/null || true
  find ./Src \( -name "*.mdhi" -o -name "*.mdhs" \) "${verbose_mode}" -delete 2>/dev/null || true

  # Additional maintenance cleanup
  log "INFO" "Removing temporary and backup files..."
  find . -name "*.backup" "${verbose_mode}" -delete 2>/dev/null || true
  find . -name "*.tmp" "${verbose_mode}" -delete 2>/dev/null || true
  find . -name ".mdh.tmp" "${verbose_mode}" -delete 2>/dev/null || true
  find . -name ".#*" "${verbose_mode}" -delete 2>/dev/null || true

  # Clean trunk cache if it exists
  if [[ -d ".trunk/cache" ]]; then
    log "INFO" "Cleaning trunk cache..."
    rm -rf ".trunk/cache" || true
  fi

  log "SUCCESS" "Deep clean completed successfully"
}

validate_config() {
  log "INFO" "Validating configuration files..."

  # Check YAML files
  for yaml_file in .github/workflows/*.yml .github/dependabot.yml; do
    if [[ -f "${PROJECT_ROOT}/${yaml_file}" ]]; then
      if command -v yamllint >/dev/null 2>&1; then
        if ! yamllint "${PROJECT_ROOT}/${yaml_file}"; then
          log "WARNING" "YAML validation failed for ${yaml_file}"
        fi
      fi
    fi
  done

  log "SUCCESS" "Configuration validation completed"
}

security_scan() {
  log "INFO" "Running basic security checks..."

  cd "${PROJECT_ROOT}"

  # Check for hardcoded secrets patterns
  if grep -r "password\|secret\|key\|token" --include="*.c" --include="*.h" --include="*.sh" . 2>/dev/null | grep -v "Scripts/maintenance.sh"; then
    log "WARNING" "Found potential hardcoded secrets patterns"
  fi

  # Check file permissions
  while IFS= read -r -d '' script_file; do
    log "WARNING" "Shell script not executable: ${script_file}"
  done < <(find . -name "*.sh" -not -executable -print0 || true)

  log "SUCCESS" "Security scan completed"
}

comprehensive() {
  log "INFO" "Running comprehensive workspace maintenance..."

  local failed_commands=()
  local warning_commands=()

  # List of all maintenance commands to run
  local commands=(
    "check_health"
    "check_versions"
    "lint_code"
    "validate_config"
    "security_scan"
  )

  for cmd in "${commands[@]}"; do
    log "INFO" "Running: ${cmd}"
    if ${cmd}; then
      log "SUCCESS" "${cmd} passed"
    else
      case $? in
      "${EXIT_WARNING}")
        warning_commands+=("${cmd}")
        log "WARNING" "${cmd} completed with warnings"
        ;;
      *)
        failed_commands+=("${cmd}")
        log "ERROR" "${cmd} failed"
        ;;
      esac
    fi
    echo # Add spacing between commands
  done

  # Summary
  if [[ ${#failed_commands[@]} -eq 0 && ${#warning_commands[@]} -eq 0 ]]; then
    log "SUCCESS" "All comprehensive maintenance checks passed ✓"
    return "${EXIT_SUCCESS}"
  elif [[ ${#failed_commands[@]} -eq 0 ]]; then
    log "WARNING" "Comprehensive maintenance completed with warnings: ${warning_commands[*]}"
    return "${EXIT_WARNING}"
  else
    log "ERROR" "Comprehensive maintenance failed. Failed commands: ${failed_commands[*]}"
    if [[ ${#warning_commands[@]} -gt 0 ]]; then
      log "WARNING" "Additionally, commands with warnings: ${warning_commands[*]}"
    fi
    return "${EXIT_FAILURE}"
  fi
}

comprehensive_with_clean() {
  log "INFO" "Running comprehensive workspace maintenance with deep clean..."

  # First run deep clean
  log "INFO" "Starting with deep clean..."

  # shellcheck disable=SC2310
  if ! clean_deep; then
    log "ERROR" "Deep clean failed"
    return "${EXIT_FAILURE}"
  fi

  echo # Add spacing

  # Then run comprehensive checks
  comprehensive
}

# Main command handling
case "${1:-help}" in
check-health) check_health ;;
lint-code) lint_code ;;
update-docs) update_docs ;;
check-versions) check_versions ;;
clean-build) clean_build ;;
clean-deep) clean_deep ;;
validate-config) validate_config ;;
security-scan) security_scan ;;
comprehensive) comprehensive ;;
comprehensive-with-clean) comprehensive_with_clean ;;
help | --help | -h) show_usage ;;
*)
  log "ERROR" "Unknown command: $1"
  show_usage
  exit "${EXIT_FAILURE}"
  ;;
esac
