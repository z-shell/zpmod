#!/usr/bin/env zsh
# Docker-based testing script for zpmod
# Usage: ./scripts/docker-test.sh [test-type]
# test-type: minimal, full-dev, smoke, build-validation, docs, docs-clean, release, release-clean, all

set -euo pipefail

SCRIPT_DIR="${0:A:h}"
PROJECT_ROOT="${SCRIPT_DIR:h}"
TEST_TYPE="${1:-all}"

cd "${PROJECT_ROOT}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
  echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
  echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
  echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
  echo -e "${RED}[ERROR]${NC} $1"
}

ensure_build_results_dir() {
  # Check if the main build-results directory exists
  if [[ ! -d "${PROJECT_ROOT}/build-results" ]]; then
    log_info "Creating build-results directory"
    mkdir -p "${PROJECT_ROOT}/build-results"
  else
    log_info "Using existing build-results directory"
  fi
  # Create subdirectories if they don't exist
  for subdir in "docs" "release"; do
    if [[ ! -d "${PROJECT_ROOT}/build-results/${subdir}" ]]; then
      log_info "Creating build-results/${subdir} directory"
      mkdir -p "${PROJECT_ROOT}/build-results/${subdir}"
    else
      log_info "Using existing build-results/${subdir} directory"
    fi
  done
}

# Export host user/group so containers can match file ownership
export_user_env() {
  USER_ID="$(id -u)"
  export USER_ID
  GROUP_ID="$(id -g)"
  export GROUP_ID
  USER_NAME="$(id -un)"
  export USER_NAME
  GROUP_NAME="$(id -gn)"
  export GROUP_NAME
  log_info "Using UID:GID ${USER_ID}:${GROUP_ID} (${USER_NAME}:${GROUP_NAME}) for container file ownership"
}

run_test() {
  local test_name="$1"
  log_info "Running ${test_name} tests..."

  if (cd docker && docker-compose run --rm "test-${test_name}"); then
    log_success "${test_name} tests passed"
    return 0
  else
    log_error "${test_name} tests failed"
    return 1
  fi
}

# Create build results directory
ensure_build_results_dir

case "${TEST_TYPE}" in
minimal)
  log_info "Testing minimal environment (CI simulation)"
  run_test minimal
  ;;
full-dev)
  log_info "Testing full development environment"
  run_test full-dev
  ;;
smoke)
  log_info "Running smoke tests"
  (cd docker && docker-compose run --rm test-smoke)
  ;;
build-validation)
  log_info "Validating build configuration"
  (cd docker && docker-compose run --rm build-validation)
  ;;
all)
  log_info "Running all test scenarios"

  echo
  log_info "=== 1. Build Validation ==="
  (cd docker && docker-compose run --rm build-validation)

  echo
  log_info "=== 2. Documentation Build ==="
  export_user_env
  (cd docker && docker-compose run --rm build-docs)

  echo
  log_info "=== 3. Smoke Tests ==="
  (cd docker && docker-compose run --rm test-smoke)

  echo
  log_info "=== 4. Minimal Environment Tests ==="
  run_test minimal

  echo
  log_info "=== 5. Full Development Environment Tests ==="
  run_test full-dev

  echo
  log_info "=== 6. Release Staging (clean) ==="
  export_user_env
  (cd docker && docker-compose run --rm build-release-clean)

  log_success "All test scenarios completed"
  ;;
docs | documentation)
  log_info "Building documentation in container"
  ensure_build_results_dir
  export_user_env
  (cd docker && docker-compose run --rm build-docs)
  log_success "Documentation build completed"

  # Check if docs were generated
  if [[ -f "${PROJECT_ROOT}/build-results/docs/html/index.html" ]]; then
    log_success "Documentation available at: build-results/docs/html/index.html"
    log_info "Open in browser: file://${PROJECT_ROOT}/build-results/docs/html/index.html"
  else
    log_warning "Documentation HTML not found in expected location"
  fi
  ;;
release)
  log_info "Building and staging release in container"
  ensure_build_results_dir
  export_user_env
  (cd docker && docker-compose run --rm build-release)
  log_success "Release staging completed"

  if [[ -f "${PROJECT_ROOT}/build-results/release/lib/zsh/site-modules/zpmod.so" || -f "${PROJECT_ROOT}/build-results/release/lib/zsh/site-modules/zpmod.bundle" || -f "${PROJECT_ROOT}/build-results/release/lib/zsh/site-modules/zpmod.dylib" ]]; then
    log_success "Staged module found under build-results/release/lib/zsh/site-modules/"
  else
    log_warning "Staged module not found in expected location; check build output"
  fi
  ;;
release-clean)
  log_info "Clean building and staging release in container"
  ensure_build_results_dir
  export_user_env
  (cd docker && docker-compose run --rm build-release-clean)
  log_success "Clean release staging completed"
  ;;
docs-clean | documentation-clean)
  log_info "Clean building documentation in container"
  ensure_build_results_dir
  export_user_env
  (cd docker && docker-compose run --rm build-docs-clean)
  log_success "Clean documentation build completed"

  # Check if docs were generated
  if [[ -f "${PROJECT_ROOT}/build-results/docs/html/index.html" ]]; then
    log_success "Documentation available at: build-results/docs/html/index.html"
    log_info "Open in browser: file://${PROJECT_ROOT}/build-results/docs/html/index.html"
  else
    log_warning "Documentation HTML not found in expected location"
  fi
  ;;
*)
  log_error "Unknown test type: ${TEST_TYPE}"
  echo "Usage: $0 [minimal|full-dev|smoke|build-validation|docs|docs-clean|all]"
  echo "       $0 [release|release-clean]"
  exit 1
  ;;
esac
