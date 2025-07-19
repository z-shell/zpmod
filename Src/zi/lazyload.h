#ifndef ZPMOD_LAZY_LOAD_H
#define ZPMOD_LAZY_LOAD_H

/**
 * Structure to hold information about a lazily loaded function
 */
typedef struct zp_lazy_function {
    char *name;                /* Function name */
    void *function_ptr;        /* Pointer to the function */
    int loaded;                /* Whether the function is loaded */
    char *library_path;        /* Path to the library containing the function */
    void *library_handle;      /* Handle to the loaded library */
} *ZpLazyFunction;

/**
 * Structure to hold the lazy loading system
 */
typedef struct zp_lazy_loader {
    ZpLazyFunction *functions; /* Array of lazy functions */
    int function_count;        /* Number of functions */
    int functions_alloc;       /* Allocated size for functions array */
    int debug_mode;            /* Debug mode flag */
} *ZpLazyLoader;

/**
 * Initialize the lazy loading system
 *
 * @return Pointer to initialized lazy loader or NULL on failure
 */
ZpLazyLoader zp_lazy_loader_init(void);

/**
 * Free all resources used by the lazy loading system
 *
 * @param loader The lazy loader to free
 */
void zp_lazy_loader_destroy(ZpLazyLoader loader);

/**
 * Register a function for lazy loading
 *
 * @param loader The lazy loader to use
 * @param name The name of the function
 * @param library_path The path to the library containing the function
 * @return 0 on success, non-zero on failure
 */
int zp_lazy_loader_register(ZpLazyLoader loader, const char *name, const char *library_path);

/**
 * Get a function pointer, loading it if necessary
 *
 * @param loader The lazy loader to use
 * @param name The name of the function
 * @return Function pointer or NULL on failure
 */
void *zp_lazy_loader_get(ZpLazyLoader loader, const char *name);

/**
 * Unload all loaded functions to free memory
 *
 * @param loader The lazy loader to use
 */
void zp_lazy_loader_unload_all(ZpLazyLoader loader);

/**
 * Enable or disable debug mode
 *
 * @param loader The lazy loader to use
 * @param debug_mode 1 to enable debug mode, 0 to disable
 */
void zp_lazy_loader_set_debug(ZpLazyLoader loader, int debug_mode);

#endif /* ZPMOD_LAZY_LOAD_H */
