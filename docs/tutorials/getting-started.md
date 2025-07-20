# Getting Started with zpmod

## Introduction

This tutorial will guide you through installing, configuring, and using the `zpmod` Zsh module. zpmod is a binary Zsh module that enhances your shell experience by automatically compiling scripts and tracking performance metrics.

## Installation

### Prerequisites

- Zsh 5.8.1 or newer
- A C compiler (gcc, clang)
- Basic build tools (make, autoconf)

### Installation Methods

#### Method 1: Using Zi (Recommended) {#installation-with-zi}

If you use the [Zi](https://github.com/z-shell/zi) plugin manager:

```zsh
zi module build
```

This command will download, compile, and install the module for you.

#### Method 2: Standalone Installation {#manual-installation}

For a manual installation:

```zsh
git clone https://github.com/z-shell/zpmod.git
cd zpmod
./Scripts/install.sh
```

Or download and run the installer directly:

```zsh
sh <(curl -fsSL https://raw.githubusercontent.com/z-shell/zpmod/main/Scripts/install.sh)
```

#### Method 3: Pre-built Binaries {#pre-built-binaries}

Download pre-compiled binaries from [releases](https://github.com/z-shell/zpmod/releases/latest):

```zsh
# Linux (x86_64)
curl -L -o zpmod.so https://github.com/z-shell/zpmod/releases/latest/download/zpmod.so

# macOS (Intel/Apple Silicon)
curl -L -o zpmod.bundle https://github.com/z-shell/zpmod/releases/latest/download/zpmod.bundle

# Install to modules directory
mkdir -p ~/.local/lib/zsh/modules/zi
mv zpmod.* ~/.local/lib/zsh/modules/zi/
```

## Configuration

### Loading the Module

Add these lines to the beginning of your `~/.zshrc`:

```zsh
# For Zi installation
module_path+=( "${HOME}/.zi/zmodules/zpmod/Src" )
zmodload zi/zpmod

# For standalone installation (path may vary)
# module_path+=( "/path/to/zpmod/installation/Src" )
# zmodload zi/zpmod
```

### Environment Variables

You can customize zpmod behavior with these environment variables:

- `ZPMOD_SKIP_PATTERNS`: Patterns to skip during compilation (requires custom build)
- `ZPMOD_DEBUG`: Enable detailed debug logging (if compiled with debug support)

### Helper Functions (Optional)

For enhanced functionality, consider setting up the zpmod configuration helpers:

```bash
# Download and set up configuration helpers
mkdir -p ~/.config/zi
curl -o ~/.config/zi/zpmod-config.zsh \
  https://raw.githubusercontent.com/z-shell/zpmod/main/Config/zpmod-config.zsh

# Add to your .zshrc after loading zpmod
[[ -f "$HOME/.config/zi/zpmod-config.zsh" ]] && source "$HOME/.config/zi/zpmod-config.zsh"
```

This provides useful commands like:

- `zpmod-stats` - Performance statistics
- `zpmod-benchmark` - Shell startup benchmarking
- `zpmod-status` - Installation verification
- `zpmod-slow-files` - Identify performance bottlenecks

For detailed information, see: [Use Configuration Helpers](../how-to/use-configuration-helpers.md)

## Usage

### Performance Analysis

Generate detailed performance reports:

```zsh
# Basic performance report
zpmod source-study

# Extended report with full paths
zpmod source-study -l
```

### Monitoring Shell Startup

Measure your shell startup time:

```zsh
time zsh -c "source ~/.zshrc; exit"
```

### Finding Performance Bottlenecks

Identify slow-loading files:

```zsh
zpmod source-study -l | grep -E '[0-9]{2,}ms'  # Files taking 10ms or more
```

## Troubleshooting

### Common Issues

#### Module Not Loading

Check your module path and file permissions:

```zsh
# Verify module path
echo $module_path | grep zpmod

# Check file permissions
ls -la ~/.local/lib/zsh/modules/zi/zpmod.*
```

#### No Data in Reports

Make sure zpmod is loaded at the beginning of your .zshrc, before other scripts are sourced.

#### Compilation Errors

zpmod now intelligently skips file descriptors, device files, and pipes during compilation.

## Advanced Usage

### Integration with Shell Scripts

For advanced users and plugin developers:

```zsh
# Check if a script was compiled
if [[ -f "$script_path.zwc" ]]; then
    echo "Script is compiled and optimized"
fi

# Export performance data
zpmod source-study -l > "$HOME/zsh-perf.txt"
```
