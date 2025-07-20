# Using zpmod Configuration Helpers

## Overview

The zpmod configuration file provides a collection of helper functions that make it easier to work with zpmod, analyze performance, and troubleshoot issues. These utilities complement the core zpmod functionality with user-friendly commands.

## Installation and Setup

### Automatic Setup (Recommended)

The advanced installer automatically sets up the configuration:

```bash
# Download and run the advanced installer
curl -s https://raw.githubusercontent.com/z-shell/zpmod/main/Scripts/advanced-install.sh | bash
```

This will:

- Download the configuration to `~/.config/zi/zpmod-config.zsh`
- Create a user customization file at `~/.config/zi/zpmod-user-config.zsh`
- Add sourcing commands to your `.zshrc`

### Manual Setup

1. **Download the configuration file:**

   ```bash
   mkdir -p ~/.config/zi
   curl -o ~/.config/zi/zpmod-config.zsh \
     https://raw.githubusercontent.com/z-shell/zpmod/main/Config/zpmod-config.zsh
   ```

2. **Source it in your `.zshrc`:**

   ```bash
   # Add to ~/.zshrc after loading zpmod
   [[ -f "$HOME/.config/zi/zpmod-config.zsh" ]] && source "$HOME/.config/zi/zpmod-config.zsh"
   ```

3. **Alternative: Direct sourcing**
   ```bash
   # Source directly from the zpmod repository
   source /path/to/zpmod/Config/zpmod-config.zsh
   ```

## Available Helper Functions

### Performance Analysis

#### `zpmod-stats`

View current performance statistics with a summary of sourced files.

```bash
zpmod-stats
```

**Example output:**

```text
=== ZPMOD Performance Statistics ===
2ms /home/user/.zshrc
15ms /home/user/.oh-my-zsh/oh-my-zsh.sh
3ms /home/user/.zsh/aliases.zsh

To generate data, source some files after loading zpmod:
  source ~/.zshrc
  source /path/to/some/script.zsh
```

#### `zpmod-detailed`

Get detailed performance reports with full file paths.

```bash
zpmod-detailed
```

This provides the same information as `zpmod source-study -l` but with user-friendly formatting.

#### `zpmod-benchmark`

Benchmark your shell startup performance over multiple runs.

```bash
zpmod-benchmark
```

**Example output:**

```text
Benchmarking shell startup performance...
Run 1: 245ms
Run 2: 251ms
Run 3: 248ms
Run 4: 252ms
Run 5: 249ms
Average startup time: 249ms
✅ Startup performance looks good
```

**Performance thresholds:**

- **Good**: < 3000ms (3 seconds)
- **Slow**: > 3000ms - Consider optimization

#### `zpmod-slow-files`

Identify files that take more than 10ms to load.

```bash
zpmod-slow-files
```

**Example output:**

```text
=== Files taking >10ms to load ===
45ms /home/user/.oh-my-zsh/plugins/git/git.plugin.zsh
23ms /home/user/.nvm/nvm.sh
15ms /home/user/.pyenv/bin/pyenv
```

### Diagnostics and Troubleshooting

#### `zpmod-status`

Comprehensive status check for zpmod installation.

```bash
zpmod-status
```

**Example output:**

```text
=== ZPMOD Status Check ===
✅ zpmod module is loaded
✅ zpmod command is available
📁 Looking for module file: zpmod.so
✅ Found: /home/user/.zi/zmodules/zpmod/Src/zi/zpmod.so
```

**Return codes:**

- `0`: Everything working correctly
- `1`: Issues detected (module not loaded or command unavailable)

## Configuration Variables

### Debug Settings

```bash
# Enable debug output (if zpmod was compiled with debug support)
export ZPMOD_DEBUG=1  # 0 = disabled (default), 1 = enabled
```

### Platform Detection

The configuration automatically detects your platform and sets the appropriate module extension:

- **macOS**: `ZPMOD_MODULE_EXT="bundle"`
- **Linux/Others**: `ZPMOD_MODULE_EXT="so"`

This variable is used by the helper functions to locate the correct module file.

## Customization

### User Configuration File

Create `~/.config/zi/zpmod-user-config.zsh` for your personal customizations:

```bash
# ZPMOD User Configuration
# Customize zpmod behavior here

# Custom thresholds
export ZPMOD_SLOW_THRESHOLD=5  # Custom slow file threshold in ms

# Custom aliases
alias zperf='zpmod-benchmark'
alias zslow='zpmod-slow-files'
alias zstatus='zpmod-status'

# Custom functions
my-zpmod-report() {
    echo "=== My Custom zpmod Report ==="
    zpmod-stats
    echo ""
    zpmod-slow-files
}
```

### Environment Integration

The configuration works well with other shell frameworks:

```bash
# For Oh My Zsh users
plugins=(... zpmod)  # If you create a zpmod plugin

# For Prezto users
zstyle ':prezto:load' pmodule 'zpmod'

# For Zi users (recommended)
zi load z-shell/zpmod
```

## Troubleshooting

### Common Issues

1. **"zpmod command not available"**

   ```bash
   # Check if module is loaded
   zmodload | grep zpmod

   # If not loaded, check module path
   echo $module_path

   # Reload zpmod
   zmodload -u zi/zpmod  # Unload
   zmodload zi/zpmod     # Reload
   ```

2. **"No statistics available yet"**

   ```bash
   # zpmod needs to track some file sourcing first
   source ~/.zshrc
   source /some/script.zsh
   zpmod-stats  # Should now show data
   ```

3. **Module file not found**

   ```bash
   # Check installation path
   find / -name "zpmod.so" -o -name "zpmod.bundle" 2>/dev/null

   # Update module_path if needed
   module_path+=("/correct/path/to/zpmod/Src")
   ```

### Debug Mode

Enable verbose output for troubleshooting:

```bash
export ZPMOD_DEBUG=1
zpmod-status  # Will show additional debug information
```

## Integration Examples

### Shell Startup Optimization Workflow

1. **Baseline measurement:**

   ```bash
   zpmod-benchmark
   ```

2. **Identify slow files:**

   ```bash
   zpmod-slow-files
   ```

3. **Detailed analysis:**

   ```bash
   zpmod-detailed | head -20  # Show top 20 slowest
   ```

4. **Optimize and re-test:**
   ```bash
   # After making changes
   zpmod-benchmark
   ```

### Automated Performance Monitoring

Add to your `.zshrc` for automatic monitoring:

```bash
# Show startup stats if shell starts slowly
if [[ -f "$HOME/.config/zi/zpmod-config.zsh" ]]; then
    source "$HOME/.config/zi/zpmod-config.zsh"

    # Optional: Show stats on slow startup
    # Uncomment the line below to enable
    # zpmod-stats
fi
```

## Advanced Usage

### Custom Performance Thresholds

Modify the helper functions for your needs:

```bash
# Custom slow file threshold
zpmod-very-slow-files() {
    echo "=== Files taking >50ms to load ==="
    if command -v zpmod >/dev/null 2>&1; then
        zpmod source-study -l 2>/dev/null | awk '$1 ~ /^[0-9]+ms$/ && $1+0 > 50'
    fi
}
```

### Integration with Monitoring Tools

```bash
# Export metrics for external monitoring
zpmod-export-metrics() {
    local metrics_file="/tmp/zpmod-metrics.json"
    {
        echo "{"
        echo "  \"startup_time\": \"$(zpmod-benchmark 2>/dev/null | grep Average | awk '{print $3}')\","
        echo "  \"slow_files_count\": \"$(zpmod-slow-files 2>/dev/null | wc -l)\","
        echo "  \"timestamp\": \"$(date -Iseconds)\""
        echo "}"
    } > "$metrics_file"
    echo "Metrics exported to: $metrics_file"
}
```

## Best Practices

1. **Load zpmod early** in your `.zshrc` for better tracking
2. **Use zpmod-benchmark regularly** to catch performance regressions
3. **Monitor slow files** after installing new plugins or tools
4. **Keep user customizations** in the separate user-config.zsh file
5. **Use zpmod-status** to verify installation after updates

## See Also

- [Getting Started with zpmod](../tutorials/getting-started.md)
- [Optimize Compilation](optimize-compilation.md)
- [Configure Path Caching](configure-path-caching.md)
- [zpmod API Reference](../reference/api.md)
