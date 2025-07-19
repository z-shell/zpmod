# zpmod Configuration File
# Place this in ~/.config/zpmod/config.zsh or source directly in .zshrc

# ============================================================================
# ZPMOD BASIC CONFIGURATION
# ============================================================================
#
# NOTE: zpmod has limited built-in configuration options. The module primarily
# works automatically with intelligent defaults. This file provides helper
# functions to work with the actual zpmod commands.

# Debug and Logging
# ----------------

# Debug level (if zpmod was compiled with debug support):
# 0 = No debug output (default)
# 1 = Basic debug output
export ZPMOD_DEBUG=${ZPMOD_DEBUG:-0}

# Platform Detection
# ------------------

case "$OSTYPE" in
    darwin*)
        # macOS specific settings
        export ZPMOD_MODULE_EXT="bundle"
        ;;
    linux*)
        # Linux specific settings
        export ZPMOD_MODULE_EXT="so"
        ;;
    *)
        # Default settings for other platforms
        export ZPMOD_MODULE_EXT="so"
        ;;
esac

# Custom Functions for Working with zpmod
# ---------------------------------------

# Function to get current zpmod statistics
zpmod-stats() {
    echo "=== ZPMOD Performance Statistics ==="
    if command -v zpmod >/dev/null 2>&1; then
        zpmod source-study 2>/dev/null || echo "No statistics available yet"
        echo ""
        echo "To generate data, source some files after loading zpmod:"
        echo "  source ~/.zshrc"
        echo "  source /path/to/some/script.zsh"
    else
        echo "❌ zpmod command not available"
        echo "Make sure zpmod is installed and loaded:"
        echo "  module_path+=(\"/path/to/zpmod/modules\")"
        echo "  zmodload zi/zpmod"
    fi
}

# Function to get detailed file paths
zpmod-detailed() {
    echo "=== ZPMOD Detailed Report (Full Paths) ==="
    if command -v zpmod >/dev/null 2>&1; then
        zpmod source-study -l 2>/dev/null || echo "No data available yet"
    else
        echo "❌ zpmod command not available"
    fi
}

# Function to benchmark shell startup
zpmod-benchmark() {
    echo "Benchmarking shell startup performance..."
    local total=0
    local runs=5

    for i in {1..$runs}; do
        local start_time=$(date +%s%3N)
        zsh -c "source ~/.zshrc; exit" 2>/dev/null
        local end_time=$(date +%s%3N)
        local duration=$((end_time - start_time))
        total=$((total + duration))
        echo "Run $i: ${duration}ms"
    done

    local average=$((total / runs))
    echo "Average startup time: ${average}ms"

    if [[ $average -gt 3000 ]]; then
        echo "⚠️  Startup time is slow (>3000ms)"
        echo "Consider running 'zpmod source-study -l' to identify slow files"
    else
        echo "✅ Startup performance looks good"
    fi
}

# Function to find slow-loading files
zpmod-slow-files() {
    echo "=== Files taking >10ms to load ==="
    if command -v zpmod >/dev/null 2>&1; then
        local slow_files=$(zpmod source-study -l 2>/dev/null | awk '$1 ~ /^[0-9]+ms$/ && $1+0 > 10')
        if [[ -n "$slow_files" ]]; then
            echo "$slow_files"
        else
            echo "No slow files found or no data available yet"
        fi
    else
        echo "❌ zpmod command not available"
    fi
}

# Function to check zpmod status
zpmod-status() {
    echo "=== ZPMOD Status Check ==="

    # Check if zpmod is loaded
    if zmodload | grep -q zpmod; then
        echo "✅ zpmod module is loaded"
    else
        echo "❌ zpmod module is not loaded"
        return 1
    fi

    # Check if command is available
    if command -v zpmod >/dev/null 2>&1; then
        echo "✅ zpmod command is available"
    else
        echo "❌ zpmod command is not available"
        return 1
    fi

    # Check module file
    local module_file
    case "$OSTYPE" in
        darwin*) module_file="zpmod.bundle" ;;
        *) module_file="zpmod.so" ;;
    esac

    echo "📁 Looking for module file: $module_file"
    for path in "${module_path[@]}"; do
        if [[ -f "$path/zi/$module_file" ]]; then
            echo "✅ Found: $path/zi/$module_file"
            return 0
        fi
    done
    echo "❌ Module file not found in module_path"
}

# Auto-setup function
zpmod-setup() {
    echo "Setting up zpmod helper functions..."

    # Check if zpmod is available
    if ! command -v zpmod >/dev/null 2>&1; then
        echo "❌ zpmod command not found."
        echo ""
        echo "To install zpmod:"
        echo "1. Download from: https://github.com/z-shell/zpmod/releases"
        echo "2. Or build from source: git clone https://github.com/z-shell/zpmod.git"
        echo "3. Load in .zshrc:"
        echo "   module_path+=(\"/path/to/zpmod/modules\")"
        echo "   zmodload zi/zpmod"
        echo ""
        echo "Available helper functions (even without zpmod):"
        echo "  zpmod-status     - Check zpmod installation status"
        echo "  zpmod-benchmark  - Benchmark shell startup time"
        return 1
    fi

    echo "✅ zpmod is available"
    echo ""
    echo "Available helper functions:"
    echo "  zpmod-stats      - View current performance data"
    echo "  zpmod-detailed   - View detailed file paths and timing"
    echo "  zpmod-benchmark  - Benchmark shell startup time"
    echo "  zpmod-slow-files - Find files taking >10ms to load"
    echo "  zpmod-status     - Check zpmod installation status"
    echo ""
    echo "Core zpmod commands:"
    echo "  zpmod source-study    - Basic performance report"
    echo "  zpmod source-study -l - Detailed report with full paths"
    echo ""
    echo "To start collecting data, make sure zpmod is loaded early in your .zshrc"
}

# Run setup automatically
zpmod-setup

# ============================================================================
# END OF CONFIGURATION
# ============================================================================

# Uncomment the following line to see performance stats on shell startup
# zpmod-stats
