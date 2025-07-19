#include "lazyload.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

/**
 * Initialize the lazy loading system
 */
ZpLazyLoader zp_lazy_loader_init(void)
{
    ZpLazyLoader loader = (ZpLazyLoader)malloc(sizeof(struct zp_lazy_loader));
    if (!loader) {
        return NULL;
    }

    loader->functions = NULL;
    loader->function_count = 0;
    loader->functions_alloc = 0;
    loader->debug_mode = 0;

    return loader;
}

/**
 * Free all resources used by the lazy loading system
 */
void zp_lazy_loader_destroy(ZpLazyLoader loader)
{
    if (!loader) {
        return;
    }

    // Unload all libraries and free function structures
    for (int i = 0; i < loader->function_count; i++) {
        ZpLazyFunction func = loader->functions[i];
        if (func) {
            if (func->library_handle) {
                dlclose(func->library_handle);
            }

            if (func->name) {
                free(func->name);
            }

            if (func->library_path) {
                free(func->library_path);
            }

            free(func);
        }
    }

    if (loader->functions) {
        free(loader->functions);
    }

    free(loader);
}

/**
 * Register a function for lazy loading
 */
int zp_lazy_loader_register(ZpLazyLoader loader, const char *name, const char *library_path)
{
    if (!loader || !name || !library_path) {
        return 1;
    }

    // Check if function is already registered
    for (int i = 0; i < loader->function_count; i++) {
        if (strcmp(loader->functions[i]->name, name) == 0) {
            // Already registered
            return 0;
        }
    }

    // Allocate or expand function array if needed
    if (loader->function_count >= loader->functions_alloc) {
        int new_size = loader->functions_alloc == 0 ? 8 : loader->functions_alloc * 2;
        ZpLazyFunction *new_funcs = (ZpLazyFunction *)realloc(
            loader->functions,
            new_size * sizeof(ZpLazyFunction)
        );

        if (!new_funcs) {
            return 1;
        }

        loader->functions = new_funcs;
        loader->functions_alloc = new_size;
    }

    // Create new function entry
    ZpLazyFunction func = (ZpLazyFunction)malloc(sizeof(struct zp_lazy_function));
    if (!func) {
        return 1;
    }

    func->name = strdup(name);
    func->library_path = strdup(library_path);
    func->function_ptr = NULL;
    func->loaded = 0;
    func->library_handle = NULL;

    if (!func->name || !func->library_path) {
        if (func->name) free(func->name);
        if (func->library_path) free(func->library_path);
        free(func);
        return 1;
    }

    // Add to array
    loader->functions[loader->function_count++] = func;

    if (loader->debug_mode) {
        fprintf(stderr, "Registered lazy function: %s from %s\n", name, library_path);
    }

    return 0;
}

/**
 * Get a function pointer, loading it if necessary
 */
void *zp_lazy_loader_get(ZpLazyLoader loader, const char *name)
{
    if (!loader || !name) {
        return NULL;
    }

    // Find the function in our registry
    ZpLazyFunction func = NULL;
    for (int i = 0; i < loader->function_count; i++) {
        if (strcmp(loader->functions[i]->name, name) == 0) {
            func = loader->functions[i];
            break;
        }
    }

    if (!func) {
        if (loader->debug_mode) {
            fprintf(stderr, "Function not registered for lazy loading: %s\n", name);
        }
        return NULL;
    }

    // If already loaded, just return the pointer
    if (func->loaded && func->function_ptr) {
        return func->function_ptr;
    }

    // Load the library if not already loaded
    if (!func->library_handle) {
        if (loader->debug_mode) {
            fprintf(stderr, "Loading library for function %s: %s\n",
                    name, func->library_path);
        }

        func->library_handle = dlopen(func->library_path, RTLD_LAZY);
        if (!func->library_handle) {
            if (loader->debug_mode) {
                fprintf(stderr, "Failed to load library for %s: %s\n",
                        name, dlerror());
            }
            return NULL;
        }
    }

    // Get the function pointer
    func->function_ptr = dlsym(func->library_handle, name);
    if (!func->function_ptr) {
        if (loader->debug_mode) {
            fprintf(stderr, "Failed to find symbol %s: %s\n",
                    name, dlerror());
        }
        return NULL;
    }

    func->loaded = 1;

    if (loader->debug_mode) {
        fprintf(stderr, "Lazily loaded function: %s\n", name);
    }

    return func->function_ptr;
}

/**
 * Unload all loaded functions to free memory
 */
void zp_lazy_loader_unload_all(ZpLazyLoader loader)
{
    if (!loader) {
        return;
    }

    for (int i = 0; i < loader->function_count; i++) {
        ZpLazyFunction func = loader->functions[i];
        if (func && func->library_handle) {
            if (loader->debug_mode) {
                fprintf(stderr, "Unloading library for function %s\n", func->name);
            }

            dlclose(func->library_handle);
            func->library_handle = NULL;
            func->function_ptr = NULL;
            func->loaded = 0;
        }
    }
}

/**
 * Enable or disable debug mode
 */
void zp_lazy_loader_set_debug(ZpLazyLoader loader, int debug_mode)
{
    if (loader) {
        loader->debug_mode = debug_mode;
    }
}
