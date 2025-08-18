/**
 * \file src/module/zpmod.pro
 * \brief Prototype stub for zpmod when building out-of-tree.
 */
#ifndef ZPMOD_PRO
#define ZPMOD_PRO

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
