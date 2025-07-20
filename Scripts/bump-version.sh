#!/usr/bin/env bash

# zpmod Version Bump Script
# ==========================
#
# This script helps maintainers bump versions following the project's
# branching and tagging guidelines.
#
# Usage: ./Scripts/bump-version.sh [TYPE] [VERSION]
#
# Examples:
#   ./Scripts/bump-version.sh patch        # Auto-increment patch version
#   ./Scripts/bump-version.sh minor        # Auto-increment minor version
#   ./Scripts/bump-version.sh major        # Auto-increment major version
#   ./Scripts/bump-version.sh set 1.2.3   # Set specific version
#

set -euo pipefail

# Configuration
VERSION_FILE="Config/zpmod-version.mk"
CHANGELOG_FILE="CHANGELOG.md"

# Colors for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

log() {
  local level="$1"
  shift
  case "${level}" in
  "INFO") echo -e "${BLUE}[INFO]${NC} $*" ;;
  "WARN") echo -e "${YELLOW}[WARN]${NC} $*" ;;
  "ERROR") echo -e "${RED}[ERROR]${NC} $*" >&2 ;;
  "SUCCESS") echo -e "${GREEN}[SUCCESS]${NC} $*" ;;
  *) echo "$*" ;;
  esac
}

show_help() {
  cat <<EOF
Usage: $0 [TYPE] [VERSION]

Version Bump Types:
  patch     Increment patch version (1.0.0 → 1.0.1)
  minor     Increment minor version (1.0.0 → 1.1.0)
  major     Increment major version (1.0.0 → 2.0.0)
  set X.Y.Z Set specific version

Examples:
  $0 patch                    # 1.0.0 → 1.0.1
  $0 minor                    # 1.0.0 → 1.1.0
  $0 major                    # 1.0.0 → 2.0.0
  $0 set 2.0.0-beta.1         # Set to 2.0.0-beta.1

Environment Variables:
  DRY_RUN=1                   Show what would be done without making changes
  SKIP_CHECKS=1               Skip git status and changelog checks

See docs/how-to/branching-and-tagging-guidelines.md for complete workflow.
EOF
}

get_current_version() {
  if [[ ! -f ${VERSION_FILE} ]]; then
    log "ERROR" "Version file not found: ${VERSION_FILE}"
    exit 1
  fi

  grep "^ZPMOD_VERSION=" "${VERSION_FILE}" | cut -d'=' -f2 | tr -d '"' | tr -d "'"
}

validate_version() {
  local version="$1"
  # Accept both 3-part (X.Y.Z) and 4-part (X.Y.Z.W) versions with optional prerelease
  if [[ ! ${version} =~ ^[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?(-[a-zA-Z0-9.-]+)?$ ]]; then
    log "ERROR" "Invalid version format: ${version}"
    log "INFO" "Expected format: X.Y.Z, X.Y.Z.W, or X.Y.Z-prerelease"
    exit 1
  fi
}

increment_version() {
  local version="$1"
  local type="$2"

  # Remove any prerelease suffix for calculations
  local base_version
  base_version="${version//-*/}"

  # Handle special case of versions like "5.9.0.1" - treat as 4-part version
  if [[ ${base_version} =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    # For 4-part versions, use the last part as patch
    local major minor patch _
    IFS='.' read -r major minor patch _ <<<"${base_version}"

    case "${type}" in
    "major")
      echo "$((major + 1)).0.0.0"
      ;;
    "minor")
      echo "${major}.$((minor + 1)).0.0"
      ;;
    "patch")
      echo "${major}.${minor}.$((patch + 1)).0"
      ;;
    *)
      log "ERROR" "Invalid increment type: ${type}"
      exit 1
      ;;
    esac
  else
    # Standard 3-part semantic version
    local major minor patch
    IFS='.' read -r major minor patch <<<"${base_version}"

    case "${type}" in
    "major")
      echo "$((major + 1)).0.0"
      ;;
    "minor")
      echo "${major}.$((minor + 1)).0"
      ;;
    "patch")
      echo "${major}.${minor}.$((patch + 1))"
      ;;
    *)
      log "ERROR" "Invalid increment type: ${type}"
      exit 1
      ;;
    esac
  fi
}

check_git_status() {
  local git_status
  git_status=$(git status --porcelain)
  if [[ -n ${git_status} ]]; then
    log "ERROR" "Working directory is not clean. Please commit or stash changes."
    git status --short
    exit 1
  fi

  local current_branch
  current_branch=$(git rev-parse --abbrev-ref HEAD)
  if [[ ${current_branch} != "develop" ]] && [[ ! ${current_branch} =~ ^release/ ]]; then
    log "WARN" "Current branch is '${current_branch}', not 'develop' or 'release/*'"
    log "INFO" "Version bumps should typically be done on develop or release branches"
  fi
}

update_version_file() {
  local new_version="$1"
  local current_version="$2"

  if [[ ${DRY_RUN:-0} == "1" ]]; then
    log "INFO" "[DRY RUN] Would update ${VERSION_FILE}: ${current_version} → ${new_version}"
    log "INFO" "[DRY RUN] Would update version constants in Src/zi/zpmod.c"
    return
  fi

  # Create backup
  cp "${VERSION_FILE}" "${VERSION_FILE}.backup"

  # Update main version
  sed -i.tmp "s/ZPMOD_VERSION=${current_version}/ZPMOD_VERSION=${new_version}/g" "${VERSION_FILE}"
  rm -f "${VERSION_FILE}.tmp" || true

  # Update version components in the file
  local major minor patch prerelease
  IFS='.' read -r major minor patch_with_pre <<<"${new_version}"

  if [[ ${patch_with_pre} == *"-"* ]]; then
    IFS='-' read -r patch prerelease <<<"${patch_with_pre}"
  else
    patch="${patch_with_pre}"
    prerelease=""
  fi

  # Update individual version components
  sed -i.tmp "s/ZPMOD_VERSION_MAJOR=.*/ZPMOD_VERSION_MAJOR=${major}/g" "${VERSION_FILE}"
  sed -i.tmp "s/ZPMOD_VERSION_MINOR=.*/ZPMOD_VERSION_MINOR=${minor}/g" "${VERSION_FILE}"
  sed -i.tmp "s/ZPMOD_VERSION_PATCH=.*/ZPMOD_VERSION_PATCH=${patch}/g" "${VERSION_FILE}"
  sed -i.tmp "s/ZPMOD_VERSION_PRERELEASE=.*/ZPMOD_VERSION_PRERELEASE=${prerelease}/g" "${VERSION_FILE}"
  local current_date
  current_date=$(date '+%B %d, %Y')
  sed -i.tmp "s/ZPMOD_VERSION_DATE=.*/ZPMOD_VERSION_DATE='${current_date}'/g" "${VERSION_FILE}"
  rm -f "${VERSION_FILE}.tmp" || true

  # Update version constants in the C source file
  local c_file="Src/zi/zpmod.c"
  if [[ -f ${c_file} ]]; then
    cp "${c_file}" "${c_file}.backup"
    sed -i.tmp "s/#define ZPMOD_VERSION .*/#define ZPMOD_VERSION \"${new_version}\"/g" "${c_file}"
    sed -i.tmp "s/#define ZPMOD_VERSION_MAJOR .*/#define ZPMOD_VERSION_MAJOR ${major}/g" "${c_file}"
    sed -i.tmp "s/#define ZPMOD_VERSION_MINOR .*/#define ZPMOD_VERSION_MINOR ${minor}/g" "${c_file}"
    sed -i.tmp "s/#define ZPMOD_VERSION_PATCH .*/#define ZPMOD_VERSION_PATCH ${patch}/g" "${c_file}"
    sed -i.tmp "s/#define ZPMOD_VERSION_PRERELEASE .*/#define ZPMOD_VERSION_PRERELEASE \"${prerelease}\"/g" "${c_file}"
    rm -f "${c_file}.tmp" || true
    log "SUCCESS" "Updated C source version constants in ${c_file}"
  fi

  # Verify update
  local updated_version
  updated_version=$(get_current_version)
  if [[ ${updated_version} != "${new_version}" ]]; then
    # Restore backup
    mv "${VERSION_FILE}.backup" "${VERSION_FILE}"
    [[ -f "${c_file}.backup" ]] && mv "${c_file}.backup" "${c_file}"
    log "ERROR" "Failed to update version file"
    exit 1
  fi

  rm -f "${VERSION_FILE}.backup"
  [[ -f "${c_file}.backup" ]] && rm -f "${c_file}.backup"
  log "SUCCESS" "Updated ${VERSION_FILE}: ${current_version} → ${new_version}"
}

update_changelog() {
  local new_version="$1"
  local current_version="$2"

  if [[ ! -f ${CHANGELOG_FILE} ]]; then
    log "WARN" "Changelog file not found: ${CHANGELOG_FILE}"
    return
  fi

  if [[ ${DRY_RUN:-0} == "1" ]]; then
    log "INFO" "[DRY RUN] Would update ${CHANGELOG_FILE} for version ${new_version}"
    return
  fi

  # Add version entry if [Unreleased] section exists
  if grep -q "## \[Unreleased\]" "${CHANGELOG_FILE}"; then
    # Get current date
    local release_date
    release_date=$(date +%Y-%m-%d)

    # Add new version section after [Unreleased]
    sed -i.tmp "/## \[Unreleased\]/a\\
\\
## [${new_version}] - ${release_date}" "${CHANGELOG_FILE}"
    rm -f "${CHANGELOG_FILE}.tmp" || true

    log "SUCCESS" "Updated ${CHANGELOG_FILE} with version ${new_version}"
  else
    log "WARN" "No [Unreleased] section found in ${CHANGELOG_FILE}"
  fi
}

commit_changes() {
  local new_version="$1"
  local current_version="$2"

  if [[ ${DRY_RUN:-0} == "1" ]]; then
    log "INFO" "[DRY RUN] Would commit version bump: ${current_version} → ${new_version}"
    return
  fi

  # Add files to git
  git add "${VERSION_FILE}"
  if [[ -f ${CHANGELOG_FILE} ]]; then
    git add "${CHANGELOG_FILE}"
  fi

  # Commit
  git commit -m "bump: version ${current_version} → ${new_version}

- Updated ${VERSION_FILE}
- Updated ${CHANGELOG_FILE}

See docs/how-to/branching-and-tagging-guidelines.md for next steps."

  log "SUCCESS" "Committed version bump: ${current_version} → ${new_version}"
  log "INFO" "Next steps:"
  # shellcheck disable=SC2016
  log "INFO" '  1. Push changes: git push origin $(git rev-parse --abbrev-ref HEAD)'
  log "INFO" "  2. Create release branch: git checkout -b release/v${new_version}"
  log "INFO" "  3. Follow tagging guidelines in docs/how-to/branching-and-tagging-guidelines.md"
}

main() {
  # Parse arguments
  local bump_type="${1-}"
  local specific_version="${2-}"

  if [[ ${bump_type} == "-h" ]] || [[ ${bump_type} == "--help" ]] || [[ -z ${bump_type} ]]; then
    show_help
    exit 0
  fi

  # Pre-flight checks
  if [[ ${SKIP_CHECKS:-0} != "1" ]]; then
    check_git_status
  fi

  # Get current version
  local current_version
  current_version=$(get_current_version)
  log "INFO" "Current version: ${current_version}"

  # Calculate new version
  local new_version
  case "${bump_type}" in
  "patch" | "minor" | "major")
    new_version=$(increment_version "${current_version}" "${bump_type}")
    ;;
  "set")
    if [[ -z ${specific_version} ]]; then
      log "ERROR" "Specific version required for 'set' command"
      show_help
      exit 1
    fi
    new_version="${specific_version}"
    ;;
  *)
    log "ERROR" "Invalid bump type: ${bump_type}"
    show_help
    exit 1
    ;;
  esac

  # Validate new version
  validate_version "${new_version}"

  if [[ ${new_version} == "${current_version}" ]]; then
    log "WARN" "New version is the same as current version: ${new_version}"
    exit 0
  fi

  log "INFO" "New version: ${new_version}"

  # Confirm if not in DRY_RUN mode
  if [[ ${DRY_RUN:-0} != "1" ]]; then
    echo -n "Proceed with version bump ${current_version} → ${new_version}? [y/N] "
    read -r confirm
    if [[ ${confirm} != "y" ]] && [[ ${confirm} != "Y" ]]; then
      log "INFO" "Version bump cancelled"
      exit 0
    fi
  fi

  # Execute version bump
  update_version_file "${new_version}" "${current_version}"
  update_changelog "${new_version}" "${current_version}"
  commit_changes "${new_version}" "${current_version}"

  log "SUCCESS" "Version bump complete: ${current_version} → ${new_version}"
}

main "$@"
