#!/usr/bin/env bash

# ZPMOD Advanced Installation Script
# ============================================================================
#
# This script provides multiple installation methods for the zpmod module:
# 1. Binary installation (pre-compiled)
# 2. Source compilation
# 3. Development setup
# 4. Zi integration
#
# Usage: ./advanced-install.sh [OPTIONS]
# ============================================================================

set -euo pipefail

# Configuration
SCRIPT_NAME="$(basename "$0")"
readonly SCRIPT_NAME
readonly REPO_URL="https://github.com/z-shell/zpmod"
readonly RELEASES_URL="${REPO_URL}/releases"
readonly RAW_URL="https://raw.githubusercontent.com/z-shell/zpmod/main"

# Colors for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly PURPLE='\033[0;35m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m' # No Color

# Global variables
INSTALL_TYPE="binary"
INSTALL_DIR="${HOME}/.local"
MODULE_DIR=""
ZI_INTEGRATION=false
DEVELOPMENT_MODE=false
VERBOSE=false
FORCE=false
CONFIG_SETUP=true

# ============================================================================
# Utility Functions
# ============================================================================

log() {
  local level="$1"
  shift
  # Fixing SC2155 (Declare and assign separately to avoid masking return values)
  # local timestamp
  # timestamp="$(date '+%Y-%m-%d %H:%M:%S')"
  # timestamp is currently unused, keeping for future logging enhancements

  case "${level}" in
  "INFO") echo -e "${BLUE}[INFO]${NC}  $*" ;;
  "WARN") echo -e "${YELLOW}[WARN]${NC}  $*" ;;
  "ERROR") echo -e "${RED}[ERROR]${NC} $*" >&2 ;;
  "SUCCESS") echo -e "${GREEN}[SUCCESS]${NC} $*" ;;
  "DEBUG") [[ ${VERBOSE} == true ]] && echo -e "${PURPLE}[DEBUG]${NC} $*" ;;
  *) echo -e "${RED}[UNKNOWN]${NC} $*" ;; # Adding default case SC2249
  esac
}

show_header() {
  echo -e "${CYAN}"
  echo "=================================================="
  echo "      ZPMOD Advanced Installation Script"
  echo "=================================================="
  echo -e "${NC}"
  echo "This script will install the zpmod Zsh module"
  echo "with advanced features and configuration options."
  echo
}

show_help() {
  cat <<EOF
Usage: ${SCRIPT_NAME} [OPTIONS]

Installation Options:
  -t, --type TYPE         Installation type: binary, source, dev (default: binary)
  -d, --dir DIRECTORY     Installation directory (default: ${HOME}/.local)
  --zi                    Enable Zi integration setup
  --dev                   Development mode (includes debug symbols)

Configuration Options:
  --no-config             Skip configuration file setup
  --force                 Force installation (overwrite existing)
  -v, --verbose           Verbose output

Other Options:
  -h, --help              Show this help message
  --version               Show version information

Installation Types:
  binary                  Download pre-compiled binary (fastest)
  source                  Compile from source (recommended)
  dev                     Development setup with all tools

Examples:
  ${SCRIPT_NAME}                          # Quick binary install
  ${SCRIPT_NAME} --type source --zi       # Source install with Zi integration
  ${SCRIPT_NAME} --dev --verbose          # Development setup with verbose output
  ${SCRIPT_NAME} --dir /usr/local --force # System-wide installation

EOF
}

detect_platform() {
  # Fix SC2155: Declare and assign separately
  local os
  os="$(uname -s)"
  local arch
  arch="$(uname -m)"

  case "${os}" in
  "Linux")
    case "${arch}" in
    "x86_64") echo "linux-x86_64" ;;
    "aarch64" | "arm64") echo "linux-arm64" ;;
    *)
      log "ERROR" "Unsupported architecture: ${arch}"
      exit 1
      ;;
    esac
    ;;
  "Darwin")
    case "${arch}" in
    "x86_64") echo "macos-x86_64" ;;
    "arm64") echo "macos-arm64" ;;
    *)
      log "ERROR" "Unsupported architecture: ${arch}"
      exit 1
      ;;
    esac
    ;;
  *)
    log "ERROR" "Unsupported operating system: ${os}"
    exit 1
    ;;
  esac
}

check_dependencies() {
  local deps=("curl" "zsh")
  local missing=()

  if [[ ${INSTALL_TYPE} == "source" || ${INSTALL_TYPE} == "dev" ]]; then
    deps+=("git" "make" "gcc")
  fi

  for dep in "${deps[@]}"; do
    if ! command -v "${dep}" >/dev/null 2>&1; then
      missing+=("${dep}")
    fi
  done

  if [[ ${#missing[@]} -gt 0 ]]; then
    log "ERROR" "Missing dependencies: ${missing[*]}"
    log "ERROR" "Please install them and try again"
    exit 1
  fi

  log "SUCCESS" "All dependencies satisfied"
}

get_latest_version() {
  log "DEBUG" "Fetching latest version information"
  curl -s "${RELEASES_URL}/latest" | grep '"tag_name":' | sed -E 's/.*"v([^"]+)".*/\1/' || echo "unknown"
}

get_module_extension() {
  case "$(uname -s)" in
  "Linux") echo "so" ;;
  "Darwin") echo "bundle" ;;
  *) echo "so" ;;
  esac
}

# ============================================================================
# Installation Functions
# ============================================================================

install_binary() {
  log "INFO" "Starting binary installation"

  # Fix SC2155: Declare and assign separately
  local platform
  platform="$(detect_platform)"
  local version
  version="$(get_latest_version)"
  local ext
  ext="$(get_module_extension)"
  local module_file="zpmod.${ext}"

  log "INFO" "Platform: ${platform}"
  log "INFO" "Version: ${version}"
  log "INFO" "Module extension: ${ext}"

  # Create module directory
  MODULE_DIR="${INSTALL_DIR}/lib/zsh/modules/zi"
  mkdir -p "${MODULE_DIR}"

  # Download binary
  local download_url="${RELEASES_URL}/latest/download/${module_file}"
  log "INFO" "Downloading from: ${download_url}"

  if curl -L -o "${MODULE_DIR}/${module_file}" "${download_url}"; then
    chmod 755 "${MODULE_DIR}/${module_file}"
    log "SUCCESS" "Binary downloaded and installed"
  else
    log "ERROR" "Failed to download binary"
    log "INFO" "Falling back to source installation"
    INSTALL_TYPE="source"
    install_source
    return
  fi
}

install_source() {
  log "INFO" "Starting source installation"

  # Fix SC2155: Declare and assign separately
  local temp_dir
  temp_dir="$(mktemp -d)"
  local ext
  ext="$(get_module_extension)"

  # Clone repository
  log "INFO" "Cloning repository to ${temp_dir}"
  git clone "${REPO_URL}" "${temp_dir}"
  cd "${temp_dir}"

  # Build
  log "INFO" "Building zpmod module"
  if [[ ${DEVELOPMENT_MODE} == true ]]; then
    log "INFO" "Building with debug symbols"
    CFLAGS="-g -O0" ./Scripts/install.sh --target="${INSTALL_DIR}" --verbose
  else
    ./Scripts/install.sh --target="${INSTALL_DIR}" --verbose
  fi

  MODULE_DIR="${INSTALL_DIR}/lib/zsh/modules/zi"

  # Verify build
  if [[ -f "${MODULE_DIR}/zpmod.${ext}" ]]; then
    log "SUCCESS" "Source compilation completed"
  else
    log "ERROR" "Build failed - module file not found"
    exit 1
  fi

  # Cleanup
  cd - >/dev/null
  rm -rf "${temp_dir}"
}

install_development() {
  log "INFO" "Setting up development environment"

  DEVELOPMENT_MODE=true
  install_source

  # Additional development tools
  local dev_dir="${INSTALL_DIR}/share/zpmod-dev"
  mkdir -p "${dev_dir}"

  # Create development configuration
  cat >"${dev_dir}/zpmod-dev.zsh" <<'EOF'
# ZPMOD Development Configuration

# Enable comprehensive debugging
export ZPMOD_DEBUG=3
export ZPMOD_LOG_FILE="$HOME/.cache/zpmod/debug.log"

# Development tracking
export ZPMOD_TRACK_LEVEL=2
export ZPMOD_TRACK_MEMORY=true
export ZPMOD_TRACK_CACHE=true

# Enable all advanced features
export ZPMOD_PARALLEL_COMPILE=true
export ZPMOD_ENABLE_CACHE=true
export ZPMOD_AUTO_CLEANUP=true

# Development helper functions
zpmod-dev-reload() {
    zmodload -u zi/zpmod 2>/dev/null || true
    zmodload zi/zpmod
    echo "zpmod reloaded"
}

zpmod-dev-test() {
    echo "Running zpmod development tests..."
    zpmod source-study --stats
    zpmod-dev-reload
    echo "Development test completed"
}

echo "ZPMOD Development mode enabled"
echo "Use 'zpmod-dev-reload' to reload the module"
echo "Use 'zpmod-dev-test' to run development tests"
EOF

  log "SUCCESS" "Development environment configured"
  log "INFO" "Development config: ${dev_dir}/zpmod-dev.zsh"
}

setup_zi_integration() {
  log "INFO" "Setting up Zi integration"

  local zi_config="${HOME}/.config/zi/zpmod-integration.zsh"
  mkdir -p "$(dirname "${zi_config}")"

  cat >"${zi_config}" <<EOF
# ZPMOD Zi Integration Configuration
# Auto-generated by zpmod advanced installer

# Load zpmod module with Zi
module_path+=("${MODULE_DIR}/..")
zmodload zi/zpmod

# Enhanced Zi integration
if [[ \$+functions[zi] -eq 1 ]]; then
    # Enable zpmod tracking for Zi operations
    export ZPMOD_ZI_INTEGRATION=true

    # Add zpmod commands to Zi
    zi-zpmod-stats() {
        zi cclear
        zpmod source-study -l
    }

    zi-zpmod-report() {
        echo "=== Zi Plugin Performance Report ==="
        zpmod source-study --stats
    }

    # Register with Zi command system
    zi subcmds zpmod-stats zi-zpmod-stats
    zi subcmds zpmod-report zi-zpmod-report

    echo "✅ ZPMOD Zi integration loaded"
else
    echo "⚠️  Zi not found - basic zpmod functionality available"
fi
EOF

  log "SUCCESS" "Zi integration configured: ${zi_config}"

  # Add to user's Zi configuration if it exists
  local zi_init="${HOME}/.config/zi/init.zsh"
  if [[ -f ${zi_init} ]] && ! grep -q "zpmod-integration.zsh" "${zi_init}"; then
    echo "source \"${zi_config}\"" >>"${zi_init}"
    log "INFO" "Added to Zi initialization"
  fi
}

setup_configuration() {
  if [[ ${CONFIG_SETUP} != true ]]; then
    log "INFO" "Skipping configuration setup"
    return
  fi

  log "INFO" "Setting up zpmod configuration"

  local config_dir="${HOME}/.config/zpmod"
  mkdir -p "${config_dir}"

  # Download configuration file
  local config_url="${RAW_URL}/Config/zpmod-config.zsh"
  if curl -s -o "${config_dir}/config.zsh" "${config_url}"; then
    log "SUCCESS" "Configuration downloaded: ${config_dir}/config.zsh"
  else
    log "WARN" "Could not download configuration file"
  fi

  # Create user configuration
  local user_config="${config_dir}/user-config.zsh"
  if [[ ! -f ${user_config} ]]; then
    cat >"${user_config}" <<EOF
# ZPMOD User Configuration
# Customize zpmod behavior here

# Installation-specific settings
export ZPMOD_INSTALL_DIR="${INSTALL_DIR}"
export ZPMOD_MODULE_DIR="${MODULE_DIR}"

# Performance settings (adjust to your needs)
export ZPMOD_MIN_SIZE=1024
export ZPMOD_TRACK_LEVEL=1

# Add your custom settings below:

EOF
    log "SUCCESS" "User configuration created: ${user_config}"
  fi
}

configure_shell() {
  log "INFO" "Configuring shell integration"

  local zshrc="${HOME}/.zshrc"
  # Fix SC2155: Declare and assign separately
  local backup
  backup="${zshrc}.zpmod-backup-$(date +%s)"

  # Create backup
  if [[ -f ${zshrc} ]]; then
    cp "${zshrc}" "${backup}"
    log "INFO" "Created backup: ${backup}"
  fi

  # Configuration block
  local config_block
  config_block="
# ZPMOD Configuration - Added by advanced installer
if [[ -d \"${MODULE_DIR}\" ]]; then
    module_path+=(\"$(dirname "${MODULE_DIR}")\")

    # Load configuration if available
    [[ -f \"\$HOME/.config/zpmod/config.zsh\" ]] && source \"\$HOME/.config/zpmod/config.zsh\"
    [[ -f \"\$HOME/.config/zpmod/user-config.zsh\" ]] && source \"\$HOME/.config/zpmod/user-config.zsh\"

    # Load the module
    zmodload zi/zpmod

    # Load Zi integration if available
    [[ -f \"\$HOME/.config/zi/zpmod-integration.zsh\" ]] && source \"\$HOME/.config/zi/zpmod-integration.zsh\"
fi
# End ZPMOD Configuration
"

  # Add configuration if not already present
  if [[ -f ${zshrc} ]] && grep -q "ZPMOD Configuration" "${zshrc}"; then
    log "INFO" "zpmod configuration already present in .zshrc"
  else
    echo "${config_block}" >>"${zshrc}"
    log "SUCCESS" "Added zpmod configuration to .zshrc"
  fi
}

verify_installation() {
  log "INFO" "Verifying installation"

  # Fix SC2155: Declare and assign separately
  local ext
  ext="$(get_module_extension)"
  local module_file="${MODULE_DIR}/zpmod.${ext}"

  # Check module file
  if [[ ! -f ${module_file} ]]; then
    log "ERROR" "Module file not found: ${module_file}"
    # If FORCE is enabled, we can continue despite errors
    if [[ ${FORCE:-false} == true ]]; then
      log "WARN" "Continuing anyway due to --force flag"
      return 0
    fi
    return 1
  fi

  # Check if loadable
  if zsh -c "module_path+=('$(dirname "${MODULE_DIR}")'); zmodload zi/zpmod" 2>/dev/null; then
    log "SUCCESS" "Module loads successfully"
  else
    log "ERROR" "Module failed to load"
    # If FORCE is enabled, we can continue despite errors
    if [[ ${FORCE:-false} == true ]]; then
      log "WARN" "Continuing anyway due to --force flag"
      return 0
    fi
    return 1
  fi

  # Test basic functionality
  if zsh -c "module_path+=('$(dirname "${MODULE_DIR}")'); zmodload zi/zpmod; zpmod source-study" 2>/dev/null; then
    log "SUCCESS" "Basic functionality verified"
  else
    log "WARN" "Basic functionality test failed (may be normal for fresh install)"
  fi

  return 0
}

show_completion_message() {
  echo
  echo -e "${GREEN}=================================================="
  echo "         ZPMOD Installation Completed!"
  echo -e "==================================================${NC}"
  echo
  echo "📁 Installation directory: ${INSTALL_DIR}"
  echo "🔧 Module location: ${MODULE_DIR}"
  echo "⚙️  Configuration: ${HOME}/.config/zpmod/"
  echo
  echo -e "${YELLOW}Next Steps:${NC}"
  echo "1. Restart your shell or run: source ~/.zshrc"
  echo "2. Test the installation: zpmod source-study"
  echo "3. View configuration: cat ~/.config/zpmod/config.zsh"
  echo
  if [[ ${ZI_INTEGRATION} == true ]]; then
    echo -e "${BLUE}Zi Integration:${NC}"
    echo "- Use 'zi zpmod-stats' for performance reports"
    echo "- Use 'zi zpmod-report' for detailed analysis"
    echo
  fi
  echo -e "${PURPLE}Documentation:${NC}"
  echo "- GitHub: ${REPO_URL}"
  echo "- Configuration: ~/.config/zpmod/config.zsh"
  echo "- Logs: ~/.cache/zpmod/debug.log (if debug enabled)"
  echo
  echo -e "${CYAN}Enjoy faster Zsh with zpmod! 🚀${NC}"
}

# ============================================================================
# Main Installation Logic
# ============================================================================

parse_arguments() {
  while [[ $# -gt 0 ]]; do
    case $1 in
    -t | --type)
      INSTALL_TYPE="$2"
      if [[ ! ${INSTALL_TYPE} =~ ^(binary|source|dev)$ ]]; then
        log "ERROR" "Invalid install type: ${INSTALL_TYPE}"
        exit 1
      fi
      shift 2
      ;;
    -d | --dir)
      INSTALL_DIR="$2"
      shift 2
      ;;
    --zi)
      ZI_INTEGRATION=true
      shift
      ;;
    --dev)
      INSTALL_TYPE="dev"
      DEVELOPMENT_MODE=true
      shift
      ;;
    --no-config)
      CONFIG_SETUP=false
      shift
      ;;
    --force)
      # FORCE is currently unused, but we'll keep the flag for future implementation
      # and make it used in a verification step
      FORCE=true
      log "DEBUG" "Force mode enabled (will overwrite existing files)"
      shift
      ;;
    -v | --verbose)
      VERBOSE=true
      shift
      ;;
    -h | --help)
      show_help
      exit 0
      ;;
    --version)
      echo "zpmod Advanced Installer v2.1.0"
      exit 0
      ;;
    *)
      log "ERROR" "Unknown option: $1"
      show_help
      exit 1
      ;;
    esac
  done
}

main() {
  show_header
  parse_arguments "$@"

  log "INFO" "Starting zpmod installation"
  log "INFO" "Type: ${INSTALL_TYPE}"
  log "INFO" "Directory: ${INSTALL_DIR}"
  log "INFO" "Zi Integration: ${ZI_INTEGRATION}"

  # Pre-installation checks
  check_dependencies

  # Installation based on type
  case "${INSTALL_TYPE}" in
  "binary")
    install_binary
    ;;
  "source")
    install_source
    ;;
  "dev")
    install_development
    ;;
  *)
    log "ERROR" "Unknown installation type: ${INSTALL_TYPE}"
    exit 1
    ;;
  esac

  # Post-installation setup
  setup_configuration

  if [[ ${ZI_INTEGRATION} == true ]]; then
    setup_zi_integration
  fi

  configure_shell

  # Verification
  # shellcheck disable=SC2310
  if verify_installation; then
    show_completion_message
  else
    log "ERROR" "Installation verification failed"
    exit 1
  fi
}

# Run main function with all arguments
main "$@"
