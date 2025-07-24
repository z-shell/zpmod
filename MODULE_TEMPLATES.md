# Module Development Templates

This directory contains complete templates and examples for creating new Zsh modules based on the zpmod architecture.

## Quick Start Template

A minimal but functional Zsh module template.

### File Structure
```
templates/simple-module/
├── simple.mdd           # Module definition
├── simple.c             # Module implementation  
├── Makefile.in          # Build configuration
├── configure.ac         # Autotools configuration
└── build.sh            # Build script
```

### simple.mdd
```makefile
name=example/simple
link=dynamic
load=no

autofeatures=""

objects="simple.o"
```

### simple.c
```c
/* -*- Mode: C; c-default-style: "linux"; c-basic-offset: 4; indent-tabs-mode: nil -*-
 * vim:sw=4:sts=4:et
 */

#include "simple.mdh"
#include "simple.pro"

/* Module state */
static int module_initialized = 0;

/* Command implementation */
static int bin_simple(char *nam, char **argv, UNUSED(Options ops), UNUSED(int func)) {
    if (OPT_ISSET(ops, 'h')) {
        printf("Usage: simple [options] [arguments]\n");
        printf("\n");
        printf("A simple example Zsh module command.\n");
        printf("\n");
        printf("Options:\n");
        printf("  -h               Show this help\n");
        printf("  -v               Verbose output\n");
        printf("\n");
        return 0;
    }
    
    int verbose = OPT_ISSET(ops, 'v');
    
    if (verbose) {
        printf("Simple module executing with %d arguments:\n", arrlen(argv));
        char **arg = argv;
        int i = 0;
        while (*arg) {
            printf("  arg[%d]: %s\n", i++, *arg++);
        }
    } else {
        printf("Simple module executed successfully.\n");
    }
    
    return 0;
}

/* Module features */
static struct builtin bintab[] = {
    BUILTIN("simple", 0, bin_simple, 0, -1, 0, "hv", NULL),
};

static struct features module_features = {
    bintab, sizeof(bintab)/sizeof(*bintab),
    NULL, 0,
    NULL, 0, 
    NULL, 0,
    0
};

/* Module lifecycle functions */

int setup_(UNUSED(Module m)) {
    module_initialized = 1;
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
    if (module_initialized) {
        printf("Simple module unloaded.\n");
        fflush(stdout);
        module_initialized = 0;
    }
    return 0;
}
```

## Advanced Template

A more comprehensive template with common features.

### advanced.c (partial)
```c
#include "advanced.mdh" 
#include "advanced.pro"

/* Module data structures */
typedef struct advanced_data {
    char *name;
    int value;
    struct advanced_data *next;
} AdvancedData;

static AdvancedData *data_list = NULL;
static HashTable advanced_hash = NULL;

/* Hash table node */
struct advanced_node {
    struct hashnode node;
    AdvancedData *data;
};

/* Utility functions */
static AdvancedData *create_data(const char *name, int value) {
    AdvancedData *data = (AdvancedData *)zalloc(sizeof(AdvancedData));
    if (!data) return NULL;
    
    data->name = ztrdup(name);
    data->value = value;
    data->next = NULL;
    
    return data;
}

static void free_data(AdvancedData *data) {
    if (data) {
        if (data->name) zsfree(data->name);
        zfree(data, sizeof(AdvancedData));
    }
}

/* Hash table management */
static HashTable create_advanced_hash(void) {
    HashTable ht = newhashtable(23, "advanced_hash", NULL);
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
    ht->freenode = free_advanced_node;
    ht->printnode = NULL;
    
    return ht;
}

static void free_advanced_node(HashNode hn) {
    struct advanced_node *node = (struct advanced_node *)hn;
    if (node->data) {
        free_data(node->data);
    }
    zfree(node, sizeof(struct advanced_node));
}

/* Command implementations */
static int cmd_add(char *nam, char **argv) {
    if (!*argv || !argv[1]) {
        zwarnnam(nam, "add requires name and value arguments");
        return 1;
    }
    
    char *name = *argv++;
    char *value_str = *argv++;
    int value = (int)zstrtol(value_str, NULL, 10);
    
    AdvancedData *data = create_data(name, value);
    if (!data) {
        zwarnnam(nam, "failed to create data structure");
        return 1;
    }
    
    /* Add to list */
    data->next = data_list;
    data_list = data;
    
    /* Add to hash table */
    if (advanced_hash) {
        struct advanced_node *node = (struct advanced_node *)zalloc(sizeof(struct advanced_node));
        if (node) {
            node->node.flags = 0;
            node->node.nam = ztrdup(name);
            node->data = data;
            addhashnode(advanced_hash, (HashNode)node);
        }
    }
    
    printf("Added: %s = %d\n", name, value);
    return 0;
}

static int cmd_list(char *nam, char **argv) {
    UNUSED(nam);
    UNUSED(argv);
    
    if (!data_list) {
        printf("No data stored.\n");
        return 0;
    }
    
    printf("Stored data:\n");
    AdvancedData *current = data_list;
    while (current) {
        printf("  %s = %d\n", current->name, current->value);
        current = current->next;
    }
    
    return 0;
}

static int cmd_get(char *nam, char **argv) {
    if (!*argv) {
        zwarnnam(nam, "get requires name argument");
        return 1;
    }
    
    char *name = *argv;
    
    if (advanced_hash) {
        struct advanced_node *node = (struct advanced_node *)gethashnode2(advanced_hash, name);
        if (node && node->data) {
            printf("%s = %d\n", node->data->name, node->data->value);
            return 0;
        }
    }
    
    zwarnnam(nam, "not found: %s", name);
    return 1;
}

static int bin_advanced(char *nam, char **argv, UNUSED(Options ops), UNUSED(int func)) {
    if (OPT_ISSET(ops, 'h')) {
        printf("Usage: advanced <command> [options]\n");
        printf("\n");
        printf("Commands:\n");
        printf("  add <name> <value>    Add data entry\n");
        printf("  list                  List all entries\n");
        printf("  get <name>           Get specific entry\n");
        printf("  clear                Clear all entries\n");
        printf("\n");
        printf("Options:\n");
        printf("  -h                   Show this help\n");
        printf("\n");
        return 0;
    }
    
    if (!*argv) {
        zwarnnam(nam, "command required, use -h for help");
        return 1;
    }
    
    char *command = *argv++;
    
    if (strcmp(command, "add") == 0) {
        return cmd_add(nam, argv);
    } else if (strcmp(command, "list") == 0) {
        return cmd_list(nam, argv);
    } else if (strcmp(command, "get") == 0) {
        return cmd_get(nam, argv);
    } else if (strcmp(command, "clear") == 0) {
        /* Clear all data */
        while (data_list) {
            AdvancedData *next = data_list->next;
            free_data(data_list);
            data_list = next;
        }
        if (advanced_hash) {
            deletehashtable(advanced_hash);
            advanced_hash = create_advanced_hash();
        }
        printf("All data cleared.\n");
        return 0;
    } else {
        zwarnnam(nam, "unknown command: %s", command);
        return 1;
    }
}

/* Module features */
static struct builtin bintab[] = {
    BUILTIN("advanced", 0, bin_advanced, 1, -1, 0, "h", NULL),
};

static struct features module_features = {
    bintab, sizeof(bintab)/sizeof(*bintab),
    NULL, 0,
    NULL, 0,
    NULL, 0,
    0
};

/* Module lifecycle */
int setup_(UNUSED(Module m)) {
    advanced_hash = create_advanced_hash();
    if (!advanced_hash) {
        zwarn("Cannot create hash table");
        return 1;
    }
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
    /* Clean up data list */
    while (data_list) {
        AdvancedData *next = data_list->next;
        free_data(data_list);
        data_list = next;
    }
    
    /* Clean up hash table */
    if (advanced_hash) {
        deletehashtable(advanced_hash);
        advanced_hash = NULL;
    }
    
    printf("Advanced module unloaded.\n");
    fflush(stdout);
    return 0;
}
```

## Build Configuration Templates

### Generic configure.ac
```autoconf
AC_INIT([your-module], [1.0.0])
AC_CONFIG_SRCDIR([Src/yourmodule.c])
AC_PREREQ([2.69])
AC_CONFIG_HEADERS([config.h])

# Basic information
echo "configuring for your-module $VERSION"

# System detection
AC_CANONICAL_HOST

# Compiler detection  
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL

# Required headers
AC_CHECK_HEADERS([stdio.h stdlib.h string.h unistd.h])

# Required functions
AC_CHECK_FUNCS([malloc free strdup])

# Libraries
AC_CHECK_LIB([dl], [dlopen])

# Zsh compatibility
AC_ARG_WITH([zsh],
    [AS_HELP_STRING([--with-zsh=PATH], [path to zsh executable])],
    [ZSH="$withval"],
    [AC_PATH_PROG([ZSH], [zsh])])

if test -z "$ZSH"; then
    AC_MSG_ERROR([zsh not found, please install zsh or specify --with-zsh])
fi

# Output files
AC_CONFIG_FILES([
    Makefile
    Src/Makefile
])

AC_OUTPUT

echo ""
echo "Configuration complete. Run 'make' to build the module."
echo ""
```

### Generic Makefile.in
```makefile
# Configuration
prefix = @prefix@
exec_prefix = @exec_prefix@
libdir = @libdir@
MODDIR = $(libdir)/zsh/modules

CC = @CC@
CFLAGS = @CFLAGS@
CPPFLAGS = @CPPFLAGS@
LDFLAGS = @LDFLAGS@
LIBS = @LIBS@

# Module-specific
DLCFLAGS = -fPIC
DLLDFLAGS = -shared
MODULE_EXT = .so

# Source files
SRCDIR = Src
SOURCES = $(SRCDIR)/yourmodule.c
OBJECTS = $(SOURCES:.c=.o)
MODULE = $(SRCDIR)/yourmodule$(MODULE_EXT)

# Build rules
all: $(MODULE)

$(MODULE): $(OBJECTS)
	$(CC) $(DLLDFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(DLCFLAGS) $(CPPFLAGS) -c $< -o $@

# Installation
install: $(MODULE)
	mkdir -p $(DESTDIR)$(MODDIR)
	cp $(MODULE) $(DESTDIR)$(MODDIR)/

# Cleanup
clean:
	rm -f $(OBJECTS) $(MODULE)

distclean: clean
	rm -f Makefile config.h config.status config.log

# Phony targets
.PHONY: all install clean distclean
```

### Universal build.sh
```bash
#!/usr/bin/env sh

# Module build script template
# Based on zpmod build.sh

set -e

# Colors
col_error="\033[31m"
col_info="\033[36m"
col_success="\033[32m"
col_rst="\033[0m"

# Default configuration
TARGET_DIR="${HOME}/.zsh/modules"
MODULE_NAME="yourmodule"
QUIET=0
VERBOSE=0
FORCE=0

usage() {
    cat <<EOF
Usage: $0 [OPTIONS]

Build and install Zsh module.

Options:
  --target DIR     Install to specific directory (default: ~/.zsh/modules)
  --module NAME    Module name (default: yourmodule)
  --quiet, -q      Suppress output
  --verbose, -v    Verbose output
  --force, -f      Force rebuild
  --help, -h       Show this help

EOF
}

info() {
    if [ "$QUIET" -eq 0 ]; then
        printf "${col_info}%s${col_rst}\n" "$1"
    fi
}

success() {
    if [ "$QUIET" -eq 0 ]; then
        printf "${col_success}%s${col_rst}\n" "$1"
    fi
}

error() {
    printf "${col_error}Error: %s${col_rst}\n" "$1" >&2
}

verbose() {
    if [ "$VERBOSE" -eq 1 ]; then
        printf "  %s\n" "$1"
    fi
}

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        --target)
            TARGET_DIR="$2"
            shift 2
            ;;
        --target=*)
            TARGET_DIR="${1#*=}"
            shift
            ;;
        --module)
            MODULE_NAME="$2"
            shift 2
            ;;
        --module=*)
            MODULE_NAME="${1#*=}"
            shift
            ;;
        --quiet|-q)
            QUIET=1
            shift
            ;;
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --force|-f)
            FORCE=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Check dependencies
check_deps() {
    for cmd in make gcc; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            error "Required command '$cmd' not found"
            exit 1
        fi
    done
}

# Main build function
build_module() {
    info "Building module: $MODULE_NAME"
    verbose "Target directory: $TARGET_DIR"
    
    # Check if configure exists
    if [ ! -f configure ]; then
        if [ -f configure.ac ]; then
            info "Running autoconf..."
            autoconf
        else
            error "No configure script or configure.ac found"
            exit 1
        fi
    fi
    
    # Configure
    if [ ! -f Makefile ] || [ "$FORCE" -eq 1 ]; then
        info "Configuring..."
        ./configure --prefix="$TARGET_DIR" || {
            error "Configuration failed"
            exit 1
        }
    fi
    
    # Build
    info "Compiling..."
    if [ "$VERBOSE" -eq 1 ]; then
        make
    else
        make >/dev/null 2>&1
    fi || {
        error "Compilation failed"
        exit 1
    }
    
    # Install
    info "Installing..."
    make install || {
        error "Installation failed"
        exit 1
    }
    
    success "Module built and installed successfully!"
    
    # Show usage instructions
    cat <<EOF

To use the module, add these lines to your ~/.zshrc:

    module_path+=( "$TARGET_DIR/lib/zsh/modules" )
    zmodload $MODULE_NAME

Then restart your shell or run: source ~/.zshrc

EOF
}

# Main execution
check_deps
build_module
```

## Testing Template

### Basic test structure
```zsh
#!/usr/bin/env zsh
# test.zsh - Basic module testing

# Test framework
failed_tests=0
total_tests=0

test_assert() {
    local description="$1"
    local command="$2"
    local expected="$3"
    
    total_tests=$((total_tests + 1))
    
    echo -n "Testing $description... "
    
    local result
    result=$(eval "$command" 2>&1)
    
    if [ "$result" = "$expected" ]; then
        echo "PASS"
    else
        echo "FAIL"
        echo "  Expected: $expected"
        echo "  Got:      $result"
        failed_tests=$((failed_tests + 1))
    fi
}

# Module loading test
module_path+=( "./Src" )
if ! zmodload yourmodule 2>/dev/null; then
    echo "FATAL: Cannot load module"
    exit 1
fi

echo "Module loaded successfully"

# Basic functionality tests
test_assert "help command" "yourmodule -h | head -1" "Usage: yourmodule [options]"
test_assert "basic execution" "yourmodule test" "Module executed with: test"

# Cleanup
zmodload -u yourmodule

# Results
echo ""
echo "Tests completed: $total_tests"
echo "Failed: $failed_tests"

if [ $failed_tests -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed."
    exit 1
fi
```

These templates provide complete, working examples for creating new Zsh modules based on the zpmod architecture and patterns.