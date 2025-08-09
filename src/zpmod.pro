/**
 * \file src/zpmod.pro
 * \brief Prototype stub for zpmod module when building out-of-tree.
 *
 * In the full zsh build, this file is generated; here we provide a minimal
 * set of prototypes for the module to enable out-of-tree builds and Doxygen
 * documentation linking.
 */
/* Minimal .pro stub; in zsh build this would be generated from .syms */
#ifndef ZPMOD_PRO
#define ZPMOD_PRO

/* Forward declarations for exported module functions that might be referenced */

/* Types and prototypes are provided by zpmod.mdh; avoid re-including zsh.h here */

/* Builtin handlers */
int bin_custom_dot(char *name, char **argv, Options ops, int func);
int bin_readarray(char *nam, char **argv, Options ops, int func);

/* zpmod command */
void zpmod_usage(void);

/* zsh module hooks */
int setup_(Module m);
int features_(Module m, char ***features);
int enables_(Module m, int **enables);
int boot_(Module m);
int cleanup_(Module m);
int finish_(Module m);

/* Custom helpers used within this module */
char *zp_build_source_report(int no_paths, int *rep_size);
char *zp_unmetafy_zalloc(const char *to_copy, int *new_len);
char *my_ztrdup_glen(const char *s, unsigned *len_ret);
void  zp_freeparamnode(HashNode hn);

#endif /* ZPMOD_PRO */
