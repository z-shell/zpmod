/**
 * \file src/zpmod_config.h
 * \brief Minimal configuration header for out-of-tree builds without autoconf.
 * \details
 * When building the module outside the full zsh tree, this header provides a
 * conservative set of feature macros typically produced by configure. It also
 * exists to make Doxygen indexing complete when the real config.h is absent.
 */

#ifndef ZPMOD_CONFIG_H
#define ZPMOD_CONFIG_H

/* Assume availability of standard headers */
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STDARG_H 1
#define HAVE_ERRNO_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_TIME_H 1
#define HAVE_PWD_H 1
#define HAVE_GRP_H 1
#define HAVE_DIRENT_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_SYS_UTSNAME_H 1
#define HAVE_LOCALE_H 1
#define HAVE_LIMITS_H 1
#define HAVE_DLFCN_H 1

/* Functions typically present on Linux */
#define HAVE_SETUID 1
#define HAVE_SETEUID 1
#define HAVE_SETGID 1
#define HAVE_SETEGID 1
#define HAVE_SETREUID 1
#define HAVE_SETREGID 1

/* term.h presence via ncurses */
#define HAVE_TERM_H 1

/* Posix signals */
#define POSIX_SIGNALS 1

/* Multibyte support typically available */
#define MULTIBYTE_SUPPORT 1

#endif /* ZPMOD_CONFIG_H */
