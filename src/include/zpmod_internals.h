#pragma once
/* SPDX-License-Identifier: MIT */
/*
 * zpmod_internals.h
 * Minimal fallback externs & prototypes for a subset of zsh internals
 * required by zpmod when the generated "zsh.mdh" (and *.epro set) is
 * not available (e.g. slim Docker images lacking zsh -dev artifacts).
 *
 * Strict rules:
 * 1. Only declare what zpmod directly references.
 * 2. Signatures & types MUST match upstream exactly (volatile, typedefs).
 * 3. Entire header is suppressed if ZSH_MDH_INCLUDED is defined.
 */
#ifndef ZPMOD_INTERNALS_FALLBACK_H
#define ZPMOD_INTERNALS_FALLBACK_H

#if !defined(ZSH_MDH_INCLUDED)

#include <stddef.h>
#include <stdio.h>

/* Forward typedefs mirroring upstream (opaque here). NOTE: do NOT redefine
 * PrintTableStats: zsh.h already typedefs it (void (*)(HashTable)) and we
 * include zsh.h before this fallback. Redefinition caused CI failures. */
typedef struct eprog *Eprog;
struct patprog; typedef struct patprog *Patprog;            /* Patprog */
struct hashtable; typedef struct hashtable *HashTable; /* HashTable */
struct hashnode; typedef struct hashnode *HashNode;    /* HashNode */
struct builtin;  typedef struct builtin *Builtin;      /* Builtin */
typedef struct funcstack *Funcstack;                   /* Funcstack */
typedef struct features *Features;                     /* Features */
typedef struct module *Module;                         /* Module (single typedef; duplicate removed) */

/* Core global variables (match volatile & types) */
extern volatile long lastval;          /* zlong lastval (assume long when unknown) */
extern volatile int retflag;
extern volatile int errflag;
extern volatile int exit_pending;
extern volatile int trap_state;        /* enum trap_state bits */
extern char opts[];                    /* options bitmap */
extern char **pparams;
extern char *argzero;
extern char **path;
extern char *scriptname;
extern char *scriptfilename;
extern int trap_return;
extern int SHIN;
extern int subsh;
extern int thisjob;
extern int loops;
extern void *cmdstack;                 /* treat opaque */
extern int cmdsp;
extern int sourcelevel;
extern Funcstack funcstack;
extern short *fdtable;

/* Hash tables */
extern HashTable shfunctab;
extern HashTable builtintab;

/* Functions (subset) */
extern char **zarrdup(char **);
extern char *ztrdup(const char *);
extern char *unmeta(char *);
extern void pushheap(void);
extern void popheap(void);
extern char *zhtricat(const char *, const char *, const char *);
extern void freearray(char **);
extern void zerrnam(const char *, const char *, ...);
extern void zwarnnam(const char *, const char *, ...);
extern void zwarn(const char *, ...);
extern void *zhalloc(size_t);
extern void *zshcalloc(size_t);
extern int movefd(int);
extern int strsfx(const char *, const char *);
extern void queue_signals(void);
extern void unqueue_signals(void);
extern HashTable newhashtable(int, char *, PrintTableStats);
extern unsigned int hasher(const char *);
extern void emptyhashtable(HashTable);
extern void addhashnode(HashTable, char *, void *);
extern HashNode removehashnode(HashTable, const char *);
extern void deletehashtable(HashTable);
extern HashNode gethashnode2(HashTable, const char *);
extern int loop(int, int);
extern void freeeprog(Eprog);
extern int bin_zcompile(char *, char **, void *, int);
extern void incrdumpcount(void *);
extern void shinbufsave(void);
extern void shinbufrestore(void);
extern int dosetopt(int, int, int, unsigned char *);
/* Missing prototype when .epro exports absent */
extern int arrlen(char **);
/* Additional internals referenced by source.c when generated exports absent */
extern struct eprog dummy_eprog;      /* placeholder Eprog */
/* NOTE: dummy_patprog1 is a macro (#define dummy_patprog1 ((Patprog) 1)) in
 * upstream zsh headers; do NOT declare it here or it expands inside an extern
 * and breaks the build (seen in Docker CI). We rely on the macro provided by
 * vendor/zsh/Src/zsh.h. */
extern void printprompt4(void);
extern void execode(Eprog, int, int, char *);
extern char *dyncat(const char *, const char *);
extern char *pwd;                     /* current working directory */
extern FILE *xtrerr;                  /* trace/error file */
extern zlong lineno;                  /* current line number */

#endif /* !ZSH_MDH_INCLUDED */

#endif /* ZPMOD_INTERNALS_FALLBACK_H */
