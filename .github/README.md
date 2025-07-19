# Module: `zpmod`

<div align="center">

[![🍎 Build (MacOS)](https://github.com/z-shell/zpmod/actions/workflows/test-macos.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/test-macos.yml)
[![🐧 Build (Linux)](https://github.com/z-shell/zpmod/actions/workflows/test-linux.yml/badge.svg)](https://github.com/z-shell/zpmod/actions/workflows/test-linux.yml)

</div><hr />

`zpmod` is a binary Zsh module that enhances the performance and capabilities of your shell. It transparently and automatically **compiles sourced scripts** and provides detailed performance metrics.

## Key Features

- **Automatic Script Compilation**: Many plugin managers do not offer compilation of plugins, the module automatically compiles scripts as they are sourced, improving performance.
- **Performance Tracking**: `zpmod` measures and records the loading times of all files sourced via the `source` or `.` builtins. This is invaluable for profiling your shell's startup time and identifying slow plugins or scripts.
- **Detailed Reporting**: The `zpmod source-study` command provides a detailed report of all sourced files, their load times, and full paths, helping you optimize your Zsh configuration.
- **Seamless Zi Integration**: When used with Zi, `zpmod` provides enhanced performance tracking for plugins and allows for easy management through the `zi module` command.

## Installation

You can install `zpmod` using Zi (recommended) or manually for a standalone setup.

### With Zi (Recommended)

If you are using the [Zi](https://github.com/z-shell/zi) plugin manager, the recommended way to install and manage `zpmod` is with the `zi module` command.

1.  **Build the module**:

    ```zsh
    zi module build zpmod
    ```

    This command will download the `zpmod` source, compile it, and install it into the correct directory for Zi to manage.

    You can see all available options for the `zi module` command by running:

    ```zsh
    zi module -h
    ```

    Available options include:

    ```
    -B,--build       →  Build the module, append --clean to run distclean.
    -h,--help        →  Show this help message.
    -I,--info        →  Display additional information.
    -r,--reset       →  Check and rebuild the module if needed.
    ```

    For example, to perform a clean build, you can use:

    ```zsh
    zi module build zpmod --clean
    ```

2.  **Follow the instructions**:
    After the build is complete, the command will output information about the module installation.

    - If you have the Zi initialization script (`$HOME/.config/zi/init.sh`), it will automatically handle the module loading.
    - If you don't have this initialization script, follow the output instructions to add the necessary lines to your `.zshrc` file.

### Standalone Installation

If you are not using Zi, you can use the provided installation script. The script will first check if `zi` is available and will prompt you to confirm that you want to proceed with a standalone installation.

1.  **Clone the repository** (optional):

    ```zsh
    git clone https://github.com/z-shell/zpmod.git
    cd zpmod
    ```

2.  **Run the installer**:
    If you cloned the repository:

    ```zsh
    ./Scripts/install.sh
    ```

    Or download and run in one step:

    ```sh
    sh <(curl -fsSL https://raw.githubusercontent.com/z-shell/zpmod/main/Scripts/install.sh)
    ```

3.  **Follow the instructions**:
    The script will guide you through the process and provide the necessary lines to add to your `.zshrc`.

## Loading the Module

After installation, add these lines at the top of your `~/.zshrc`:

```zsh
# For Zi installation (adjust the path if you installed to a custom location)
module_path+=( "${HOME}/.zi/zmodules/zpmod/Src" )
zmodload zi/zpmod

# For standalone installation (the path will be provided by the installer)
# module_path+=( "/path/to/your/zpmod/installation/Src" )
# zmodload zi/zpmod
```

The module should be loaded at the beginning of your `.zshrc` file to ensure it can track all sourced files during shell startup.

## Usage

Once installed and loaded, `zpmod` works in the background to track sourced files and compile them. You can get a performance report at any time.

### Profiling Your Shell

To see a report of all sourced files and their loading times, run:

```zsh
zpmod source-study
```

This will output a table with the duration (in milliseconds), file name, and directory of each sourced file.

To see full paths to the files, use the `-l` flag:

```zsh
zpmod source-study -l
```

This information can help you identify which plugins or scripts are slowing down your shell's startup.

## Debugging

To enable debug messages from the module, set:

```zsh
typeset -g ZI_MOD_DEBUG=1
```

This can help diagnose issues with module loading or operation.

## System Requirements

- Zsh version 5.8.1 or newer
- GCC or compatible compiler
- Make
- Git (optional, can be skipped with the `--no-git` option to the installer)

## Troubleshooting

If you encounter build issues:

1. Use `--verbose` to see detailed build output
2. Check the `make.log` file in the build directory
3. Make sure your Zsh version is compatible (5.8.1+)
4. Try with `--clean` to perform a fresh build
5. Submit an issue with the error messages on the [GitHub repository](https://github.com/z-shell/zpmod/issues)

## Contributing

Contributions are welcome! Here's how you can help:

1. **Reporting Bugs**: Open an issue describing the bug, steps to reproduce, and your environment
2. **Suggesting Features**: Open an issue describing the feature you'd like to see
3. **Code Contributions**:
   - Fork the repository
   - Create your feature branch (`git checkout -b feature/amazing-feature`)
   - Commit your changes (`git commit -am 'Add some amazing feature'`)
   - Push to the branch (`git push origin feature/amazing-feature`)
   - Open a Pull Request

If you need to sync with a newer version of Zsh, use the `Scripts/copy_from_zsh_src.zsh` script with the path to your Zsh source.

## License

The zpmod module is available under the same license as Zsh itself. The full license text can be found in the [LICENSE](LICENSE) file.
