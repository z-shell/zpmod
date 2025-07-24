# Zsh Module Development Guide - Based on zpmod

This guide extracts and documents all the essential functionality from the zpmod module to serve as a comprehensive reference for creating new Zsh modules.

## Table of Contents

1. [Module Architecture Overview](#module-architecture-overview)
2. [File Structure](#file-structure)
3. [Module Definition File (.mdd)](#module-definition-file-mdd)
4. [Core Module Structure](#core-module-structure)
5. [Builtin Commands](#builtin-commands)
6. [Module Lifecycle Functions](#module-lifecycle-functions)
7. [Build System Integration](#build-system-integration)
8. [Installation and Loading](#installation-and-loading)
9. [Development Patterns](#development-patterns)
10. [Complete Example Template](#complete-example-template)

## Module Architecture Overview

Zsh modules are dynamically loadable shared libraries that extend Zsh functionality. The zpmod module demonstrates:

- **Binary module compilation**: Creates `.so` files loaded with `zmodload`
- **Builtin command registration**: Adds new commands to Zsh
- **Hook into existing functionality**: Overrides built-in commands (dot/source)
- **Data structure management**: Custom hash tables and data structures
- **Memory management**: Proper allocation/deallocation patterns
- **Cross-platform compatibility**: Works across different Unix systems

## File Structure

### Essential Files for a Zsh Module

```
Src/
├── yourmodule/
│   ├── yourmodule.mdd        # Module definition
│   └── yourmodule.c          # Module implementation
├── Makefile.in               # Build configuration
├── Makemod.in.in            # Module-specific build rules
└── zsh.h                     # Zsh headers (from zpmod)
```

### Supporting Files

```
├── configure.ac              # Autotools configuration
├── build.sh                  # Build script
├── config.h.in              # Configuration template
└── Test/                     # Test files (optional)
```

## Module Definition File (.mdd)

The `.mdd` file defines module metadata and build parameters:

```makefile
# Example: yourmodule.mdd
name=yournamespace/yourmodule
link=dynamic
load=no

autofeatures=""

objects="yourmodule.o"
```

### Key Parameters

- `name`: Module namespace and name (e.g., `zi/zpmod`)
- `link=dynamic`: Creates a shared library
- `load=no`: Module is loaded on demand
- `objects`: List of object files to compile
- `autofeatures`: Automatic feature detection (usually empty)

## Core Module Structure

### Essential Headers and Includes

```c
#include "yourmodule.mdh"  // Generated module header
#include "yourmodule.pro"  // Generated prototypes

// Standard Zsh includes
#include "../zsh.h"         // Core Zsh definitions
```

### Data Structures

The zpmod module demonstrates several key patterns:

#### Custom Data Structures
```c
// Example: Source event tracking
struct source_event {
    int id;
    long ts;
    char *dir_path;
    char *file_name;
    char *full_path;
    double duration;
    int load_error;
};

// Hash table node wrapper
struct zp_sevent_node {
    struct hashnode node;
    struct source_event event;
};

typedef struct zp_sevent_node *SEventNode;
```

#### Hash Table Management
```c
static HashTable custom_hash_table = NULL;

// Create hash table
HashTable zp_createhashtable(char *name) {
    HashTable ht = newhashtable(23, name, NULL);
    if (!ht) return NULL;
    
    ht->hash = hasher;
    ht->emptytable = emptyhashtable;
    ht->filltable = NULL;
    ht->cmpnodes = strcmp;
    ht->addnode = addhashnode;
    ht->getnode = gethashnode2;
    ht->getnode2 = gethashnode2;
    ht->removenode = removehashnode;
    ht->disablenode = NULL;
    ht->enablenode = NULL;
    ht->freenode = zp_free_sevent_node;
    ht->printnode = NULL;
    
    return ht;
}
```

## Builtin Commands

### Command Registration

```c
// Define builtin commands
static struct builtin bintab[] = {
    BUILTIN("yourcommand", 0, bin_yourcommand, 1, -1, 0, NULL, NULL),
    BUILTIN("zpmod", 0, bin_zpmod, 0, -1, 0, "h", NULL),
};
```

### Command Implementation Pattern

```c
// Standard builtin function signature
int bin_yourcommand(char *nam, char **argv, UNUSED(Options ops), UNUSED(int func)) {
    char *subcmd = NULL;
    int ret = 0;

    // Handle help option
    if (OPT_ISSET(ops, 'h')) {
        yourcommand_usage();
        return 0;
    }

    // Validate arguments
    if (!*argv) {
        zwarnnam(nam, "command requires arguments");
        return 1;
    }

    subcmd = *argv++;

    // Handle subcommands
    if (0 == strcmp(subcmd, "subcommand1")) {
        // Implementation
        ret = handle_subcommand1(nam, argv);
    } else if (0 == strcmp(subcmd, "subcommand2")) {
        // Implementation  
        ret = handle_subcommand2(nam, argv);
    } else {
        zwarnnam(nam, "unknown command: %s", subcmd);
        ret = 1;
    }

    return ret;
}
```

### Usage Functions

```c
void yourcommand_usage() {
    printf("Usage: yourcommand <subcommand> [options]\n");
    printf("\n");
    printf("Subcommands:\n");
    printf("  subcommand1       Description of subcommand1\n");
    printf("  subcommand2       Description of subcommand2\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h               Show this help\n");
    fflush(stdout);
}
```

## Module Lifecycle Functions

All Zsh modules must implement these core lifecycle functions:

### 1. setup_() - Module Initialization

```c
int setup_(UNUSED(Module m)) {
    // Initialize module-specific data structures
    // Set up hook overrides
    // Create hash tables or other resources
    
    // Example: Override existing builtins
    Builtin bn = (Builtin)builtintab->getnode2(builtintab, ".");
    original_dot_handler = bn->handlerfunc;
    bn->handlerfunc = custom_dot_handler;

    // Create private data structures
    if (!(custom_hash = zp_createhashtable("custom_data"))) {
        zwarn("Cannot create hash table");
        return 1;
    }

    return 0;
}
```

### 2. features_() - Feature Registration

```c
int features_(Module m, char ***features) {
    *features = featuresarray(m, &module_features);
    return 0;
}
```

### 3. enables_() - Feature Enabling

```c
int enables_(Module m, int **enables) {
    return handlefeatures(m, &module_features, enables);
}
```

### 4. boot_() - Module Boot

```c
int boot_(UNUSED(Module m)) {
    // Minimal boot function - often empty
    return 0;
}
```

### 5. cleanup_() - Feature Cleanup

```c
int cleanup_(Module m) {
    return setfeatureenables(m, &module_features, NULL);
}
```

### 6. finish_() - Module Cleanup

```c
int finish_(UNUSED(Module m)) {
    // Restore original functionality
    Builtin bn = (Builtin)builtintab->getnode2(builtintab, ".");
    bn->handlerfunc = original_dot_handler;

    // Clean up resources
    if (custom_hash) {
        deletehashtable(custom_hash);
        custom_hash = NULL;
    }

    printf("yourmodule unloaded\n");
    fflush(stdout);
    return 0;
}
```

## Build System Integration

### Configure.ac Essentials

The zpmod configure.ac provides a comprehensive template. Key sections:

```autoconf
AC_INIT
AC_CONFIG_SRCDIR([Src/zsh.h])
AC_PREREQ([2.69])
AC_CONFIG_HEADERS([config.h])

# Version information
. ${srcdir}/Config/version.mk
echo "configuring for zsh $VERSION"

# System detection
AC_CANONICAL_HOST

# Compiler detection
AC_PROG_CC
AC_PROG_CPP

# Library detection
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([m], [pow])

# Generate output files
AC_CONFIG_FILES([
    Makefile
    Src/Makefile
    Test/Makefile
])
AC_OUTPUT
```

### Makefile Structure

Key components from zpmod's Makefile.in:

```makefile
# Basic configuration
prefix = @prefix@
exec_prefix = @exec_prefix@
bindir = @bindir@
libdir = @libdir@

CC = @CC@
CFLAGS = @CFLAGS@
LDFLAGS = @LDFLAGS@
LIBS = @LIBS@

# Module-specific flags
DLCFLAGS = @DLCFLAGS@    # Position-independent code
DLLDFLAGS = @DLLDFLAGS@  # Shared library creation

# Build targets
all: modules

modules:
	cd Src && $(MAKE) modules

install: install.modules

clean:
	cd Src && $(MAKE) clean
```

## Installation and Loading

### Build Script Template

Based on zpmod's build.sh:

```bash
#!/usr/bin/env sh

# Configuration options
TARGET_DIR="${HOME}/.zsh/modules/yourmodule"
QUIET_MODE=0
VERBOSE_MODE=0

# Build function
build_module() {
    if [ ! -f configure ]; then
        error "No configure script found. Run from module directory."
        return 1
    fi

    # Configure
    ./configure --prefix="$TARGET_DIR" || return 1
    
    # Build
    make || return 1
    
    # Install
    make install || return 1
    
    info "Module built successfully in $TARGET_DIR"
    show_loading_instructions
}

show_loading_instructions() {
    cat <<EOF
To load the module, add these lines to your ~/.zshrc:

    module_path+=( "$TARGET_DIR/lib" )
    zmodload yournamespace/yourmodule

EOF
}
```

### Loading in Zsh

```zsh
# Add module directory to module path
module_path+=( "${HOME}/.zsh/modules/yourmodule/lib" )

# Load the module
zmodload yournamespace/yourmodule

# Use module commands
yourcommand subcommand1
```

## Development Patterns

### Memory Management

```c
// Use Zsh memory functions
char *buffer = zalloc(size);           // Allocate
char *string = ztrdup(source);         // Duplicate string
zfree(buffer, size);                   // Free with size
zsfree(string);                        // Free string

// Unmetafy for system calls
char *unmetafied = zp_unmetafy_zalloc(metafied_string, &length);
// Use unmetafied...
zfree(unmetafied, length);
```

### Error Handling

```c
// Warning messages
zwarnnam(command_name, "warning message: %s", argument);

// Error messages  
zerrnam(command_name, "error message: %s", argument);

// Debug messages (if debug enabled)
if (debug_enabled) {
    fprintf(stderr, "Debug: %s\n", message);
}
```

### Option Processing

```c
// Check for options in builtin
if (OPT_ISSET(ops, 'h')) {
    show_help();
    return 0;
}

// Process additional arguments
while (*argv) {
    if (**argv == '-') {
        // Handle option
        handle_option(*argv);
    } else {
        // Regular argument
        process_argument(*argv);
    }
    argv++;
}
```

## Complete Example Template

Here's a minimal but complete module template based on zpmod patterns:

### example.mdd
```makefile
name=example/demo
link=dynamic
load=no

autofeatures=""

objects="example.o"
```

### example.c
```c
#include "example.mdh"
#include "example.pro"

/* Module data structures */
static HashTable example_data = NULL;

/* Command implementations */
static int bin_example(char *nam, char **argv, UNUSED(Options ops), UNUSED(int func)) {
    if (OPT_ISSET(ops, 'h')) {
        printf("Usage: example [options]\n");
        printf("  -h    Show this help\n");
        return 0;
    }
    
    printf("Example module command executed\n");
    return 0;
}

/* Module features */
static struct builtin bintab[] = {
    BUILTIN("example", 0, bin_example, 0, -1, 0, "h", NULL),
};

static struct features module_features = {
    bintab, sizeof(bintab)/sizeof(*bintab),
    NULL, 0,
    NULL, 0,
    NULL, 0,
    0
};

/* Lifecycle functions */
int setup_(UNUSED(Module m)) {
    // Initialize module
    return 0;
}

int features_(Module m, char ***features) {
    *features = featuresarray(m, &module_features);
    return 0;
}

int enables_(Module m, int **enables) {
    return handlefeatures(m, &module_features, enables);
}

int boot_(UNUSED(Module m)) {
    return 0;
}

int cleanup_(Module m) {
    return setfeatureenables(m, &module_features, NULL);
}

int finish_(UNUSED(Module m)) {
    printf("example module unloaded\n");
    return 0;
}
```

This guide provides all the essential patterns and structures needed to create new Zsh modules based on the comprehensive zpmod implementation.