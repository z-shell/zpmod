# Internal Analysis of `zpmod`

This document provides a deeper look into the internal implementation of the `zmodload` and `zcompile` commands.

## `zcompile`: From Script to Wordcode

The `zcompile` command is responsible for taking a Zsh script or function and compiling it into a compact, binary format known as "wordcode". This process is primarily handled within the `Src/parse.c` file.

### The `bin_zcompile` Function

The entry point for the `zcompile` builtin is the `bin_zcompile` function. Its main responsibilities are:

1.  **Parsing Options**: It processes command-line options like `-k`, `-z`, `-M`, `-R`, `-t`, etc., to determine the compilation mode (KornShell vs. Zsh autoloading), memory mapping strategy, and other behaviors.
2.  **Dispatching to Builder Functions**: Based on the options, it calls one of two main functions:
    - `build_dump()`: This function is used when compiling a list of script files into a `.zwc` file. It reads each file, parses it into an `Eprog` (executable program structure), and then writes the compiled wordcode to the output file.
    - `build_cur_dump()`: This function is used with the `-c` (current session functions) or `-a` (autoloadable functions) flags. It iterates through the shell's internal function table (`shfunctab`), finds the specified functions, and compiles them.
3.  **Error Handling**: It performs checks for illegal option combinations and handles file I/O errors.

### The Wordcode Format (`.zwc`)

A `.zwc` file is not a native machine code binary. Instead, it's a custom bytecode format ("wordcode") that the Zsh interpreter can execute much more efficiently than a raw text script.

The key components of a `.zwc` file are:

1.  **Header**: Contains a magic number (`FD_MAGIC`) to identify the file type, the Zsh version it was compiled with, and metadata about the functions contained within. To handle different system architectures, the file actually contains two headers and two copies of the wordcode: one for the native byte order and one for the swapped byte order.
2.  **Function Descriptions**: For each function, there's a header (`struct fdhead`) that stores its name, the offset to its wordcode, and other flags.
3.  **Wordcode Section**: This is the sequence of `wordcode` instructions that represent the parsed logic of the script (loops, conditionals, commands, etc.).
4.  **String Table**: A separate section containing all the literal strings used in the script. The wordcode instructions reference strings by their offset in this table.

When Zsh "runs" a `.zwc` file, it's not executing machine instructions directly. It's feeding the wordcode into its own internal execution engine (`exec.c`), which interprets the codes and performs the corresponding actions. This is much faster than re-parsing the text script every time.

## `zmodload`: The Module Management System

The `zmodload` functionality, located in `Src/module.c`, manages the lifecycle of Zsh's dynamically loadable modules.

### The `bin_zmodload` Function

This is the entry point for the `zmodload` command. It acts as a dispatcher based on the provided options:

- **Loading/Unloading**: If called with a module name (e.g., `zmodload zsh/math`), it calls `load_module()`. If called with `-u`, it calls `unload_module()`.
- **Listing**: If called with no arguments, it lists the loaded modules. Options like `-b`, `-c`, `-p` modify this to list builtins, conditions, or parameters from those modules.
- **Feature Management (`-F`)**: This is handled by `bin_zmodload_features()`, which calls `handlefeatures()` to enable or disable specific features within a module.
- **Aliasing (`-a`)**: Handled by `bin_zmodload_alias()`, which creates an alias that, when called, will trigger the loading of the specified module.

### The Loading Process (`load_module`)

When `load_module` is called:

1.  **Path Searching**: It searches the `$module_path` for a file matching the module name (e.g., `zsh/math.so`).
2.  **Dynamic Linking**: It uses the system's dynamic linker (`dlopen()`) to load the shared object (`.so`) file into the shell's address space.
3.  **Setup Function**: It looks for a special "setup" function within the loaded module (e.g., `setup_zsh_math`). This function is responsible for registering the module's new commands, parameters, and other features with the Zsh core.
4.  **Dependency Management**: Modules can declare dependencies on other modules. `zmodload` ensures that all required dependencies are loaded first.

The module system is a powerful extension mechanism that allows Zsh's functionality to be expanded without recompiling the main shell binary.
