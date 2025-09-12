/**
 * \file src/module/zpmod.pro
 * \brief Prototype stub for zpmod when building out-of-tree.
 */
#ifndef ZPMOD_PRO
#define ZPMOD_PRO

#include "zpmod_utils.h"
#include "zpmod_source.h"
#include "zpmod_fpath.h"
#include "zpmod_source_hot.h"
#include "zpmod_fs.h"

/* Builtin handlers */
int bin_custom_dot(char *name, char **argv, Options ops, int func);
/* readarray builtin (implemented in builtins/readarray.c) */
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

/* Builtin command handlers */
extern int cmd_dirlist(char *nam, char **argv);
extern int cmd_pathstat(char *nam, char **argv);
extern int cmd_readfile(char *nam, char **argv);
int zp_path_warmup_core(const char *nam, int quiet, int prune_missing, int dry_run);
int cmd_source_study(char *nam, char **argv);
extern int cmd_fpath_index(char *nam, char **argv);
extern int cmd_source_hot(char *nam, char **argv);

/* Utility functions */
extern char *zp_unmetafy_zalloc(const char *to_copy, int *len);

#endif /* ZPMOD_PRO */
