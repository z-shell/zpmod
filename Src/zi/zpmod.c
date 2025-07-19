/* -*- Mode: C; c-default-style: "linux"; c-basic-offset: 4; indent-tabs-mode: nil -*-
 * vim:sw=4:sts=4:et
 *
 * zpmod.c – module for zpmod plugin manager
 *
 * Copyright (c) 2017 Sebastian Gniazdowski
 * All rights reserved.
 *
 * The file contains code copied from Zshell source (e.g. code of builtins that are
 * then customized by me) and this code is under license:
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and to distribute modified versions of this software for any
 * purpose, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * In no event shall Paul Falstad or the Zsh Development Group be liable
 * to any party for direct, indirect, special, incidental, or consequential
 * damages arising out of the use of this software and its documentation,
 * even if Paul Falstad and the Zsh Development Group have been advised of
 * the possibility of such damage.
 *
 * Paul Falstad and the Zsh Development Group specifically disclaim any
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.  The software
 * provided hereunder is on an "as is" basis, and Paul Falstad and the
 * Zsh Development Group have no obligation to provide maintenance,
 * support, updates, enhancements, or modifications.
 */

#include "zpmod.mdh"
#include "zpmod.pro"
#include "pathcache.h"
#include "compileconfig.h"
#include "lazyload.h"

/* Source/bin_dot related data structures {{{ */
static HandlerFunc originalDot = NULL, originalSource = NULL;
static HashTable zp_source_events = NULL;
static int zp_sevent_count = 0;

/* Global path cache */
static ZpPathCache zp_path_cache = NULL;
#define ZP_CACHE_SIZE 1024      /* Size of path cache hash table */
#define ZP_CACHE_LIFETIME 30    /* Cache entry lifetime in seconds */

/* Global compilation configuration */
static ZpCompileConfig zp_compile_config = NULL;

/* Global lazy loader */
static ZpLazyLoader zp_lazy_loader = NULL;

struct source_event
{
	int id;
	long ts;
	char *dir_path;
	char *file_name;
	char *full_path;
	double duration;
	int load_error;
};

struct zp_sevent_node
{
	struct hashnode node;
	struct source_event event;
};

typedef struct zp_sevent_node *SEventNode;
/* }}} */

/* Option support {{{ */
static int zp_opt_for_zsh_version[256] = {0};

enum
{
	OPT_INVALID__,
	ALIASESOPT__,
	ALIASFUNCDEF__,
	ALLEXPORT__,
	ALWAYSLASTPROMPT__,
	ALWAYSTOEND__,
	APPENDHISTORY__,
	AUTOCD__,
	AUTOCONTINUE__,
	AUTOLIST__,
	AUTOMENU__,
	AUTONAMEDIRS__,
	AUTOPARAMKEYS__,
	AUTOPARAMSLASH__,
	AUTOPUSHD__,
	AUTOREMOVESLASH__,
	AUTORESUME__,
	BADPATTERN__,
	BANGHIST__,
	BAREGLOBQUAL__,
	BASHAUTOLIST__,
	BASHREMATCH__,
	BEEP__,
	BGNICE__,
	BRACECCL__,
	BSDECHO__,
	CASEGLOB__,
	CASEMATCH__,
	CBASES__,
	CDABLEVARS__,
	CHASEDOTS__,
	CHASELINKS__,
	CHECKJOBS__,
	CHECKRUNNINGJOBS__,
	CLOBBER__,
	APPENDCREATE__,
	COMBININGCHARS__,
	COMPLETEALIASES__,
	COMPLETEINWORD__,
	CORRECT__,
	CORRECTALL__,
	CONTINUEONERROR__,
	CPRECEDENCES__,
	CSHJUNKIEHISTORY__,
	CSHJUNKIELOOPS__,
	CSHJUNKIEQUOTES__,
	CSHNULLCMD__,
	CSHNULLGLOB__,
	DEBUGBEFORECMD__,
	EMACSMODE__,
	EQUALS__,
	ERREXIT__,
	ERRRETURN__,
	EXECOPT__,
	EXTENDEDGLOB__,
	EXTENDEDHISTORY__,
	EVALLINENO__,
	FLOWCONTROL__,
	FORCEFLOAT__,
	FUNCTIONARGZERO__,
	GLOBOPT__,
	GLOBALEXPORT__,
	GLOBALRCS__,
	GLOBASSIGN__,
	GLOBCOMPLETE__,
	GLOBDOTS__,
	GLOBSTARSHORT__,
	GLOBSUBST__,
	HASHCMDS__,
	HASHDIRS__,
	HASHEXECUTABLESONLY__,
	HASHLISTALL__,
	HISTALLOWCLOBBER__,
	HISTBEEP__,
	HISTEXPIREDUPSFIRST__,
	HISTFCNTLLOCK__,
	HISTFINDNODUPS__,
	HISTIGNOREALLDUPS__,
	HISTIGNOREDUPS__,
	HISTIGNORESPACE__,
	HISTLEXWORDS__,
	HISTNOFUNCTIONS__,
	HISTNOSTORE__,
	HISTREDUCEBLANKS__,
	HISTSAVEBYCOPY__,
	HISTSAVENODUPS__,
	HISTSUBSTPATTERN__,
	HISTVERIFY__,
	HUP__,
	IGNOREBRACES__,
	IGNORECLOSEBRACES__,
	IGNOREEOF__,
	INCAPPENDHISTORY__,
	INCAPPENDHISTORYTIME__,
	INTERACTIVE__,
	INTERACTIVECOMMENTS__,
	KSHARRAYS__,
	KSHAUTOLOAD__,
	KSHGLOB__,
	KSHOPTIONPRINT__,
	KSHTYPESET__,
	KSHZEROSUBSCRIPT__,
	LISTAMBIGUOUS__,
	LISTBEEP__,
	LISTPACKED__,
	LISTROWSFIRST__,
	LISTTYPES__,
	LOCALLOOPS__,
	LOCALOPTIONS__,
	LOCALPATTERNS__,
	LOCALTRAPS__,
	LOGINSHELL__,
	LONGLISTJOBS__,
	MAGICEQUALSUBST__,
	MAILWARNING__,
	MARKDIRS__,
	MENUCOMPLETE__,
	MONITOR__,
	MULTIBYTE__,
	MULTIFUNCDEF__,
	MULTIOS__,
	NOMATCH__,
	NOTIFY__,
	NULLGLOB__,
	NUMERICGLOBSORT__,
	OCTALZEROES__,
	OVERSTRIKE__,
	PATHDIRS__,
	PATHSCRIPT__,
	PIPEFAIL__,
	POSIXALIASES__,
	POSIXARGZERO__,
	POSIXBUILTINS__,
	POSIXCD__,
	POSIXIDENTIFIERS__,
	POSIXJOBS__,
	POSIXSTRINGS__,
	POSIXTRAPS__,
	PRINTEIGHTBIT__,
	PRINTEXITVALUE__,
	PRIVILEGED__,
	PROMPTBANG__,
	PROMPTCR__,
	PROMPTPERCENT__,
	PROMPTSP__,
	PROMPTSUBST__,
	PUSHDIGNOREDUPS__,
	PUSHDMINUS__,
	PUSHDSILENT__,
	PUSHDTOHOME__,
	RCEXPANDPARAM__,
	RCQUOTES__,
	RCS__,
	RECEXACT__,
	REMATCHPCRE__,
	RESTRICTED__,
	RMSTARSILENT__,
	RMSTARWAIT__,
	SHAREHISTORY__,
	SHFILEEXPANSION__,
	SHGLOB__,
	SHINSTDIN__,
	SHNULLCMD__,
	SHOPTIONLETTERS__,
	SHORTLOOPS__,
	SHWORDSPLIT__,
	SINGLECOMMAND__,
	SINGLELINEZLE__,
	SOURCETRACE__,
	SUNKEYBOARDHACK__,
	TRANSIENTRPROMPT__,
	TRAPSASYNC__,
	TYPESETSILENT__,
	UNSET__,
	VERBOSE__,
	VIMODE__,
	WARNCREATEGLOBAL__,
	WARNNESTEDVAR__,
	XTRACE__,
	USEZLE__,
	DVORAK__,
	OPT_SIZE__
};

struct zp_option_name
{
	const char *name;
	int enum_val;
};

static struct zp_option_name zp_options[] = {
    {"aliases", ALIASESOPT__},
    {"aliasfuncdef", ALIASFUNCDEF__},
    {"allexport", ALLEXPORT__},
    {"alwayslastprompt", ALWAYSLASTPROMPT__},
    {"alwaystoend", ALWAYSTOEND__},
    {"appendcreate", APPENDCREATE__},
    {"appendhistory", APPENDHISTORY__},
    {"autocd", AUTOCD__},
    {"autocontinue", AUTOCONTINUE__},
    {"autolist", AUTOLIST__},
    {"automenu", AUTOMENU__},
    {"autonamedirs", AUTONAMEDIRS__},
    {"autoparamkeys", AUTOPARAMKEYS__},
    {"autoparamslash", AUTOPARAMSLASH__},
    {"autopushd", AUTOPUSHD__},
    {"autoremoveslash", AUTOREMOVESLASH__},
    {"autoresume", AUTORESUME__},
    {"badpattern", BADPATTERN__},
    {"banghist", BANGHIST__},
    {"bareglobqual", BAREGLOBQUAL__},
    {"bashautolist", BASHAUTOLIST__},
    {"bashrematch", BASHREMATCH__},
    {"beep", BEEP__},
    {"bgnice", BGNICE__},
    {"braceccl", BRACECCL__},
    {"bsdecho", BSDECHO__},
    {"caseglob", CASEGLOB__},
    {"casematch", CASEMATCH__},
    {"cbases", CBASES__},
    {"cprecedences", CPRECEDENCES__},
    {"cdablevars", CDABLEVARS__},
    {"chasedots", CHASEDOTS__},
    {"chaselinks", CHASELINKS__},
    {"checkjobs", CHECKJOBS__},
    {"checkrunningjobs", CHECKRUNNINGJOBS__},
    {"clobber", CLOBBER__},
    {"combiningchars", COMBININGCHARS__},
    {"completealiases", COMPLETEALIASES__},
    {"completeinword", COMPLETEINWORD__},
    {"continueonerror", CONTINUEONERROR__},
    {"correct", CORRECT__},
    {"correctall", CORRECTALL__},
    {"cshjunkiehistory", CSHJUNKIEHISTORY__},
    {"cshjunkieloops", CSHJUNKIELOOPS__},
    {"cshjunkiequotes", CSHJUNKIEQUOTES__},
    {"cshnullcmd", CSHNULLCMD__},
    {"cshnullglob", CSHNULLGLOB__},
    {"debugbeforecmd", DEBUGBEFORECMD__},
    {"emacs", EMACSMODE__},
    {"equals", EQUALS__},
    {"errexit", ERREXIT__},
    {"errreturn", ERRRETURN__},
    {"exec", EXECOPT__},
    {"extendedglob", EXTENDEDGLOB__},
    {"extendedhistory", EXTENDEDHISTORY__},
    {"evallineno", EVALLINENO__},
    {"flowcontrol", FLOWCONTROL__},
    {"forcefloat", FORCEFLOAT__},
    {"functionargzero", FUNCTIONARGZERO__},
    {"glob", GLOBOPT__},
    {"globalexport", GLOBALEXPORT__},
    {"globalrcs", GLOBALRCS__},
    {"globassign", GLOBASSIGN__},
    {"globcomplete", GLOBCOMPLETE__},
    {"globdots", GLOBDOTS__},
    {"globstarshort", GLOBSTARSHORT__},
    {"globsubst", GLOBSUBST__},
    {"hashcmds", HASHCMDS__},
    {"hashdirs", HASHDIRS__},
    {"hashexecutablesonly", HASHEXECUTABLESONLY__},
    {"hashlistall", HASHLISTALL__},
    {"histallowclobber", HISTALLOWCLOBBER__},
    {"histbeep", HISTBEEP__},
    {"histexpiredupsfirst", HISTEXPIREDUPSFIRST__},
    {"histfcntllock", HISTFCNTLLOCK__},
    {"histfindnodups", HISTFINDNODUPS__},
    {"histignorealldups", HISTIGNOREALLDUPS__},
    {"histignoredups", HISTIGNOREDUPS__},
    {"histignorespace", HISTIGNORESPACE__},
    {"histlexwords", HISTLEXWORDS__},
    {"histnofunctions", HISTNOFUNCTIONS__},
    {"histnostore", HISTNOSTORE__},
    {"histsubstpattern", HISTSUBSTPATTERN__},
    {"histreduceblanks", HISTREDUCEBLANKS__},
    {"histsavebycopy", HISTSAVEBYCOPY__},
    {"histsavenodups", HISTSAVENODUPS__},
    {"histverify", HISTVERIFY__},
    {"hup", HUP__},
    {"ignorebraces", IGNOREBRACES__},
    {"ignoreclosebraces", IGNORECLOSEBRACES__},
    {"ignoreeof", IGNOREEOF__},
    {"incappendhistory", INCAPPENDHISTORY__},
    {"incappendhistorytime", INCAPPENDHISTORYTIME__},
    {"interactive", INTERACTIVE__},
    {"interactivecomments", INTERACTIVECOMMENTS__},
    {"ksharrays", KSHARRAYS__},
    {"kshautoload", KSHAUTOLOAD__},
    {"kshglob", KSHGLOB__},
    {"kshoptionprint", KSHOPTIONPRINT__},
    {"kshtypeset", KSHTYPESET__},
    {"kshzerosubscript", KSHZEROSUBSCRIPT__},
    {"listambiguous", LISTAMBIGUOUS__},
    {"listbeep", LISTBEEP__},
    {"listpacked", LISTPACKED__},
    {"listrowsfirst", LISTROWSFIRST__},
    {"listtypes", LISTTYPES__},
    {"localoptions", LOCALOPTIONS__},
    {"localloops", LOCALLOOPS__},
    {"localpatterns", LOCALPATTERNS__},
    {"localtraps", LOCALTRAPS__},
    {"login", LOGINSHELL__},
    {"longlistjobs", LONGLISTJOBS__},
    {"magicequalsubst", MAGICEQUALSUBST__},
    {"mailwarning", MAILWARNING__},
    {"markdirs", MARKDIRS__},
    {"menucomplete", MENUCOMPLETE__},
    {"monitor", MONITOR__},
    {"multibyte", MULTIBYTE__},
    {"multifuncdef", MULTIFUNCDEF__},
    {"multios", MULTIOS__},
    {"nomatch", NOMATCH__},
    {"notify", NOTIFY__},
    {"nullglob", NULLGLOB__},
    {"numericglobsort", NUMERICGLOBSORT__},
    {"octalzeroes", OCTALZEROES__},
    {"overstrike", OVERSTRIKE__},
    {"pathdirs", PATHDIRS__},
    {"pathscript", PATHSCRIPT__},
    {"pipefail", PIPEFAIL__},
    {"posixaliases", POSIXALIASES__},
    {"posixargzero", POSIXARGZERO__},
    {"posixbuiltins", POSIXBUILTINS__},
    {"posixcd", POSIXCD__},
    {"posixidentifiers", POSIXIDENTIFIERS__},
    {"posixjobs", POSIXJOBS__},
    {"posixstrings", POSIXSTRINGS__},
    {"posixtraps", POSIXTRAPS__},
    {"printeightbit", PRINTEIGHTBIT__},
    {"printexitvalue", PRINTEXITVALUE__},
    {"privileged", PRIVILEGED__},
    {"promptbang", PROMPTBANG__},
    {"promptcr", PROMPTCR__},
    {"promptpercent", PROMPTPERCENT__},
    {"promptsp", PROMPTSP__},
    {"promptsubst", PROMPTSUBST__},
    {"pushdignoredups", PUSHDIGNOREDUPS__},
    {"pushdminus", PUSHDMINUS__},
    {"pushdsilent", PUSHDSILENT__},
    {"pushdtohome", PUSHDTOHOME__},
    {"rcexpandparam", RCEXPANDPARAM__},
    {"rcquotes", RCQUOTES__},
    {"rcs", RCS__},
    {"recexact", RECEXACT__},
    {"rematchpcre", REMATCHPCRE__},
    {"restricted", RESTRICTED__},
    {"rmstarsilent", RMSTARSILENT__},
    {"rmstarwait", RMSTARWAIT__},
    {"sharehistory", SHAREHISTORY__},
    {"shfileexpansion", SHFILEEXPANSION__},
    {"shglob", SHGLOB__},
    {"shinstdin", SHINSTDIN__},
    {"shnullcmd", SHNULLCMD__},
    {"shoptionletters", SHOPTIONLETTERS__},
    {"shortloops", SHORTLOOPS__},
    {"shwordsplit", SHWORDSPLIT__},
    {"singlecommand", SINGLECOMMAND__},
    {"singlelinezle", SINGLELINEZLE__},
    {"sourcetrace", SOURCETRACE__},
    {"sunkeyboardhack", SUNKEYBOARDHACK__},
    {"transientrprompt", TRANSIENTRPROMPT__},
    {"trapsasync", TRAPSASYNC__},
    {"typesetsilent", TYPESETSILENT__},
    {"unset", UNSET__},
    {"verbose", VERBOSE__},
    {"vi", VIMODE__},
    {"warncreateglobal", WARNCREATEGLOBAL__},
    {"warnnestedvar", WARNNESTEDVAR__},
    {"xtrace", XTRACE__},
    {"zle", USEZLE__},
    {"dvorak", DVORAK__},
    /* Below follow *aliases*, i.e. not-main, alternate option names */
    /* There are 10 uncommented entries */
    /* {"braceexpand",         -IGNOREBRACES__}, */
    {"dotglob", GLOBDOTS__},
    {"hashall", HASHCMDS__},
    {"histappend", APPENDHISTORY__},
    {"histexpand", BANGHIST__},
    /* {"log",                 -HISTNOFUNCTIONS__}, */
    {"mailwarn", MAILWARNING__},
    {"onecmd", SINGLECOMMAND__},
    {"physical", CHASELINKS__},
    {"promptvars", PROMPTSUBST__},
    {"stdin", SHINSTDIN__},
    {"trackall", HASHCMDS__},
    {NULL, 0}};
/* }}} */

/* Copied, repeated Zsh macros, data structures, etc. {{{ */
#define FD_EXT ".zwc"
#define FD_MINMAP 4096

#define FD_PRELEN 12
#define FD_MAGIC 0x04050607
#define FD_OMAGIC 0x07060504

#define FDF_MAP 1
#define FDF_OTHER 2

typedef struct fdhead *FDHead;

struct fdhead
{
	wordcode start; /* offset to function definition */
	wordcode len;	/* length of wordcode/strings */
	wordcode npats; /* number of patterns needed */
	wordcode strs;	/* offset to strings */
	wordcode hlen;	/* header length (incl. name) */
	wordcode flags; /* flags and offset to name tail */
};

#define fdheaderlen(f) (((Wordcode)(f))[FD_PRELEN])

#define fdmagic(f) (((Wordcode)(f))[0])
#define fdsetbyte(f, i, v) \
	((((unsigned char *)(((Wordcode)(f)) + 1))[i]) = ((unsigned char)(v)))
#define fdbyte(f, i) ((wordcode)(((unsigned char *)(((Wordcode)(f)) + 1))[i]))
#define fdflags(f) fdbyte(f, 0)
#define fdsetflags(f, v) fdsetbyte(f, 0, v)
#define fdother(f) (fdbyte(f, 1) + (fdbyte(f, 2) << 8) + (fdbyte(f, 3) << 16))
#define fdsetother(f, o)                               \
	do                                             \
	{                                              \
		fdsetbyte(f, 1, ((o)&0xff));           \
		fdsetbyte(f, 2, (((o) >> 8) & 0xff));  \
		fdsetbyte(f, 3, (((o) >> 16) & 0xff)); \
	} while (0)
#define fdversion(f) ((char *)((f) + 2))

#define firstfdhead(f) ((FDHead)(((Wordcode)(f)) + FD_PRELEN))
#define nextfdhead(f) ((FDHead)(((Wordcode)(f)) + (f)->hlen))

#define fdhflags(f) (((FDHead)(f))->flags)
#define fdhtail(f) (((FDHead)(f))->flags >> 2)
#define fdhbldflags(f, t) ((f) | ((t) << 2))

#define FDHF_KSHLOAD 1
#define FDHF_ZSHLOAD 2

#define fdname(f) ((char *)(((FDHead)(f)) + 1))
/* }}} */

/*
 * Compatibility functions (i.e. support for multiple Zsh versions)
 */

/* STATIC FUNCTION: zp_setup_options_table {{{ */
/**/
static void zp_setup_options_table()
{
	int i, optno;
	for (i = 0; i < sizeof(zp_options) / sizeof(struct zp_option_name) - 10 - 1; ++i)
	{
		optno = optlookup(zp_options[i].name);
		zp_opt_for_zsh_version[zp_options[i].enum_val] = optno;
	}
}
/* }}} */
/* STATIC FUNCTION: zp_conv_opt {{{ */
/**/
static int zp_conv_opt(int zp_opt_num)
{
	int sign;
	sign = zp_opt_num >= 0 ? 1 : -1;
	return sign * zp_opt_for_zsh_version[sign * zp_opt_num];
}
/* }}} */

/*
 * `.' and `source' overload (profiling loading times)
 */

/* FUNCTION: bin_custom_dot {{{ */
/**/
int bin_custom_dot(char *name, char **argv, UNUSED(Options ops), UNUSED(int func))
{
	char **old, *old0 = NULL;
	int diddot = 0, dotdot = 0;
	char *s, **t, *enam, *arg0, *buf;
	struct stat st;
	enum source_return ret;

	if (!*argv)
		return 0;
	old = pparams;
	/* get arguments for the script */
	if (argv[1])
		pparams = zarrdup(argv + 1);

	enam = arg0 = ztrdup(*argv);
	if (isset(zp_conv_opt(FUNCTIONARGZERO__)))
	{
		old0 = argzero;
		argzero = ztrdup(arg0);
	}
	s = unmeta(enam);
	errno = ENOENT;
	ret = SOURCE_NOT_FOUND;
	/* for source only, check in current directory first */
	if (*name != '.' && access(s, F_OK) == 0 && stat(s, &st) >= 0 && !S_ISDIR(st.st_mode))
	{
		diddot = 1;
		ret = custom_source(enam);
	}
	if (ret == SOURCE_NOT_FOUND)
	{
		/* use a path with / in it */
		for (s = arg0; *s; s++)
			if (*s == '/')
			{
				if (*arg0 == '.')
				{
					if (arg0 + 1 == s)
						++diddot;
					else if (arg0[1] == '.' && arg0 + 2 == s)
						++dotdot;
				}
				ret = custom_source(arg0);
				break;
			}
		if (!*s || (ret == SOURCE_NOT_FOUND &&
			    isset(zp_conv_opt(PATHDIRS__)) && diddot < 2 && dotdot == 0))
		{
			pushheap();
			/* search path for script */
			for (t = path; *t; t++)
			{
				if (!(*t)[0] || ((*t)[0] == '.' && !(*t)[1]))
				{
					if (diddot)
						continue;
					diddot = 1;
					buf = dupstring(arg0);
				}
				else
					buf = zhtricat(*t, "/", arg0);

				s = unmeta(buf);
				if (access(s, F_OK) == 0 && stat(s, &st) >= 0 && !S_ISDIR(st.st_mode))
				{
					ret = custom_source(enam = buf);
					break;
				}
			}
			popheap();
		}
	}
	/* clean up and return */
	if (argv[1])
	{
		freearray(pparams);
		pparams = old;
	}
	if (ret == SOURCE_NOT_FOUND)
	{
		if (isset(zp_conv_opt(POSIXBUILTINS__)))
		{
			/* hard error in POSIX (we'll exit later) */
			zerrnam(name, "%d: %e: %s", __LINE__, errno, enam);
		}
		else
		{
			zwarnnam(name, "%d: %e: %s", __LINE__, errno, enam);
		}
	}
	zsfree(arg0);
	if (old0)
	{
		zsfree(argzero);
		argzero = old0;
	}
	return ret == SOURCE_OK ? lastval : 128 - ret;
}
/* }}} */
/* FUNCTION: custom_source {{{ */
/**/
mod_export enum source_return
custom_source(char *s)
{
	Eprog prog;
	int tempfd = -1, fd, cj;
	zlong oldlineno;
	int oldshst, osubsh, oloops;
	char *old_scriptname = scriptname, *us;
	char *old_scriptfilename = scriptfilename;
	unsigned char *ocs;
	int ocsp;
	int otrap_return = trap_return, otrap_state = trap_state;
	struct funcstack fstack;
	enum source_return ret = SOURCE_OK;

	/* ZP-CODE */
	SEventNode zp_node;
	struct timeval zp_tv;
	struct timezone zp_dummy_tz;
	double zp_prev_tv;
	zp_tv.tv_sec = zp_tv.tv_usec = 0;
	gettimeofday(&zp_tv, &zp_dummy_tz);
	zp_prev_tv = ((((double)zp_tv.tv_sec) * 1000.0) + (((double)zp_tv.tv_usec) / 1000.0));

	if (!s ||
	    (!(prog = custom_try_source_file((us = unmeta(s)))) &&
	     (tempfd = movefd(open(us, O_RDONLY | O_NOCTTY))) == -1))
	{
		return SOURCE_NOT_FOUND;
	}

	/* save the current shell state */
	fd = SHIN;				  /* store the shell input fd                  */
	osubsh = subsh;				  /* store whether we are in a subshell        */
	cj = thisjob;				  /* store our current job number              */
	oldlineno = lineno;			  /* store our current lineno                  */
	oloops = loops;				  /* stored the # of nested loops we are in    */
	oldshst = opts[zp_conv_opt(SHINSTDIN__)]; /* store current value of this option        */
	ocs = cmdstack;
	ocsp = cmdsp;
	cmdstack = (unsigned char *)zalloc(CMDSTACKSZ);
	cmdsp = 0;

	if (!prog)
	{
		SHIN = tempfd;
		shinbufsave();
	}
	subsh = 0;
	lineno = 1;
	loops = 0;
	dosetopt(zp_conv_opt(SHINSTDIN__), 0, 1, opts);
	scriptname = s;
	scriptfilename = s;

	if (isset(zp_conv_opt(SOURCETRACE__)))
	{
		printprompt4();
		fprintf(xtrerr ? xtrerr : stderr, "<sourcetrace>\n");
	}

	/*
	 * The special return behaviour of traps shouldn't
	 * trigger in files sourced from traps; the return
	 * is just a return from the file.
	 */
	trap_state = TRAP_STATE_INACTIVE;

	sourcelevel++;

	fstack.name = scriptfilename;
	fstack.caller = funcstack ? funcstack->name : dupstring(old_scriptfilename ? old_scriptfilename : "zsh");
	fstack.flineno = 0;
	fstack.lineno = oldlineno;
	fstack.filename = scriptfilename;
	fstack.prev = funcstack;
	fstack.tp = FS_SOURCE;
	funcstack = &fstack;

	if (prog)
	{
		pushheap();
		errflag &= ~ERRFLAG_ERROR;
		execode(prog, 1, 0, "filecode");
		popheap();
		if (errflag)
			ret = SOURCE_ERROR;
	}
	else
	{
		int value;
		/* loop through the file to be sourced  */
		switch (value = loop(0, 0))
		{
		case LOOP_OK:
			/* nothing to do but compilers like a complete enum */
			break;

		case LOOP_EMPTY:
			/* Empty code resets status */
			lastval = 0;
			break;

		case LOOP_ERROR:
			ret = SOURCE_ERROR;
			break;
		}
	}

	funcstack = funcstack->prev;
	sourcelevel--;

	trap_state = otrap_state;
	trap_return = otrap_return;

	/* restore the current shell state */
	if (prog)
		freeeprog(prog);
	else
	{
		close(SHIN);
		fdtable[SHIN] = FDT_UNUSED;
		SHIN = fd;	  /* the shell input fd                   */
		shinbufrestore(); /* file handle for buffered shell input */
	}
	subsh = osubsh;					      /* whether we are in a subshell         */
	thisjob = cj;					      /* current job number                   */
	lineno = oldlineno;				      /* our current lineno                   */
	loops = oloops;					      /* the # of nested loops we are in      */
	dosetopt(zp_conv_opt(SHINSTDIN__), oldshst, 1, opts); /* SHINSTDIN option               */
	errflag &= ~ERRFLAG_ERROR;
	if (!exit_pending)
		retflag = 0;
	scriptname = old_scriptname;
	scriptfilename = old_scriptfilename;
	zfree(cmdstack, CMDSTACKSZ);
	cmdstack = ocs;
	cmdsp = ocsp;

	/* ZP-CODE */
	zp_tv.tv_sec = zp_tv.tv_usec = 0;
	gettimeofday(&zp_tv, &zp_dummy_tz);
	zp_node = (SEventNode)zshcalloc(sizeof(struct zp_sevent_node));

	if (zp_node)
	{
		char zp_tmp[20], bkp;
		char *dir_path, *file_name, *full_path, *slash;
		int is_dot_slash;

		/* Prepare paths */
		if (s[0] == '/')
		{
			/* event.full_path */
			full_path = ztrdup(s);
		}
		else
		{
			int pwd_len, rel_len;
			is_dot_slash = (s[0] == '.' && s[1] == '/');
			/* event.full_path */
			pwd_len = strlen(pwd);
			rel_len = strlen(s) - is_dot_slash * 2;
			full_path = (char *)zalloc(sizeof(char) * (pwd_len + rel_len + 2));
			strcpy(full_path, pwd);
			strcat(full_path, "/");
			strcat(full_path, s + is_dot_slash * 2);
		}

		/* event.file_name */
		slash = strrchr(full_path, '/');
		file_name = ztrdup(slash + 1);
		/* event.dir_path */
		bkp = slash[1];
		slash[1] = '\0';
		dir_path = ztrdup(full_path);
		slash[1] = bkp;

		/* Fill and add zp_node */
		++zp_sevent_count;
		zp_node->event.id = zp_sevent_count;
		zp_node->event.ts = (long)zp_prev_tv;
		zp_node->event.dir_path = dir_path;
		zp_node->event.file_name = file_name;
		zp_node->event.full_path = full_path;
		zp_node->event.duration = ((((double)zp_tv.tv_sec) * 1000.0) + (((double)zp_tv.tv_usec) / 1000.0)) - zp_prev_tv;
		zp_node->event.load_error = ret;

		sprintf(zp_tmp, "%d", zp_node->event.id);
		zp_tmp[19] = '\0';

		addhashnode(zp_source_events, ztrdup(zp_tmp), (void *)zp_node);
	}

	return ret;
}
/* }}} */
/* STATIC FUNCTION: zp_should_skip_compilation {{{ */
/**/
static int
zp_should_skip_compilation(const char *file, const struct stat *file_stat)
{
	if (!file)
		return 1;

	/* Skip file descriptor paths like /proc/self/fd/X */
	if (strncmp(file, "/proc/self/fd/", 14) == 0)
		return 1;

	/* Skip standard device files */
	if (strcmp(file, "/dev/stdin") == 0 ||
	    strcmp(file, "/dev/stdout") == 0 ||
	    strcmp(file, "/dev/stderr") == 0)
		return 1;

	/* Check if compilation is globally disabled */
	if (zp_compile_config && !zp_compile_config->enabled)
		return 1;

	/* Check if file is in the inclusion list (always compile) */
	if (zp_compile_config && zp_compile_config_should_include(zp_compile_config, file))
		return 0;

	/* Check if file should be excluded based on patterns */
	if (zp_compile_config && zp_compile_config_should_exclude(zp_compile_config, file))
		return 1;

	/* Skip if file doesn't exist or isn't a regular file */
	if (file_stat) {
		/* Use the provided stat struct */
		if (!S_ISREG(file_stat->st_mode))
			return 1;
	} else if (zp_path_cache) {
		/* Use our path cache */
		if (!zp_path_cache_is_regular(zp_path_cache, file))
			return 1;
	} else {
		/* Fall back to direct stat if cache not initialized */
		struct stat st;
		if (stat(file, &st) != 0 || !S_ISREG(st.st_mode)) {
			return 1;
		}
	}

	return 0;
}
/* }}} */
/* FUNCTION: custom_try_source_file {{{ */
/**/
Eprog custom_try_source_file(char *file)
{
	Eprog prog;
	struct stat stc, stn;
	int rc, rn, faltered = 0, flen;
	char *wc, *tail, *file_dup;

	if ((tail = strrchr(file, '/')))
		tail++;
	else
		tail = file;

	if (strsfx(FD_EXT, file))
	{
		queue_signals();
		prog = custom_check_dump_file(file, NULL, tail, NULL, 0);
		unqueue_signals();
		return prog;
	}
	wc = dyncat(file, FD_EXT);

	/* Use path cache for file stats if available */
	if (zp_path_cache) {
		rc = zp_path_cache_stat(zp_path_cache, wc, &stc);
		rn = zp_path_cache_stat(zp_path_cache, file, &stn);
	} else {
		rc = stat(wc, &stc);
		rn = stat(file, &stn);
	}

	/* ZP-CODE */
	if (file != tail)
	{
		faltered = 1;
		*--tail = '\0';
	}
	file_dup = ztrdup(file);
	flen = strlen(file);
	if (faltered)
	{
		*tail++ = '/';
	}
	/* If there is no zwc file, or if it is less recent than script file */
	int has_write_access = (access(file_dup, W_OK) == 0);
	int is_debug_mode = zp_compile_config ? zp_compile_config->debug_mode : 0;

	if ((!rn && (rc || (stc.st_mtime < stn.st_mtime))) &&
	    !zp_should_skip_compilation(file, &stn) &&
	    (has_write_access || is_debug_mode))
	{
		/* Check if batch mode is enabled */
		if (zp_compile_config && zp_compile_config->batch_mode) {
			/* Add to batch for later compilation */
			zp_compile_config_add_pending(zp_compile_config, file);

			/* Check if the batch should be processed now */
			zp_compile_config_process_batch(zp_compile_config);
		} else {
			/* Immediate compilation */
			char *args[] = {file, NULL};
			struct options ops;

			/* Initialise options structure */
			memset(ops.ind, 0, MAX_OPS * sizeof(unsigned char));
			ops.args = NULL;
			ops.argscount = ops.argsalloc = 0;
			ops.ind['U'] = 1;

			/* Invoke compilation */
			if (access(file, R_OK) == 0 && access(file, F_OK) == 0 &&
				0 != strcmp(file, "/dev/null") && 0 != strcmp(file, "./"))
			{
				bin_zcompile("ZIModule_", args, &ops, 0);
			}
			else
			{
				if (0 == strcmp(
						getsparam("ZI_MOD_DEBUG") ? getsparam("ZI_MOD_DEBUG") : "0",
						"1"))
				{
					zwarnnam("ZIModule",
							"%d: Couldn't read the script: `%s', compilation skipped",
							__LINE__, file);
				}
			}
		}

		/* Repeat stat for newly created zwc */
		rc = stat(wc, &stc);
	}

	zfree(file_dup, flen);

	queue_signals();
	if (!rc && (rn || stc.st_mtime >= stn.st_mtime) &&
	    (prog = custom_check_dump_file(wc, &stc, tail, NULL, 0)))
	{
		unqueue_signals();
		return prog;
	}
	unqueue_signals();
	return NULL;
}

/* }}} */

/* Code copied from Zshell's parse.c {{{ */
/**/
#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MMAP) && defined(HAVE_MUNMAP)

#include <sys/mman.h>

/**/
#if defined(MAP_SHARED) && defined(PROT_READ)

/**/
#define USE_MMAP 1

/**/
#endif
/**/
#endif

/**/
#ifdef USE_MMAP

/* List of dump files mapped. */

static FuncDump dumps;
/* }}} */
/* STATIC FUNCTION: custom_zwcstat {{{ */
/**/
static int
custom_zwcstat(char *filename, struct stat *buf)
{
	/* If we have a path cache, use it */
	if (zp_path_cache) {
		int result = zp_path_cache_stat(zp_path_cache, filename, buf);
		if (result == 0)
			return 0;
	} else {
		/* Fall back to direct stat if cache not initialized */
		if (stat(filename, buf) == 0)
			return 0;
	}

#ifdef HAVE_FSTAT
	FuncDump f;

	for (f = dumps; f; f = f->next)
	{
		if (!strncmp(filename, f->filename, strlen(f->filename)) &&
		    !fstat(f->fd, buf))
			return 0;
	}
#endif
	return 1;
}
/* }}} */
/* STATIC FUNCTION: custom_load_dump_file {{{ */
/* Load a dump file (i.e. map it). */
static void
custom_load_dump_file(char *dump, struct stat *sbuf, int other, int len)
{
	FuncDump d;
	Wordcode addr;
	int fd, off, mlen;

	if (other)
	{
		static size_t pgsz = 0;

		if (!pgsz)
		{

#ifdef _SC_PAGESIZE
			pgsz = sysconf(_SC_PAGESIZE); /* SVR4 */
#else
#ifdef _SC_PAGE_SIZE
			pgsz = sysconf(_SC_PAGE_SIZE); /* HPUX */
#else
			pgsz = getpagesize();
#endif
#endif

			pgsz--;
		}
		off = len & ~pgsz;
		mlen = len + (len - off);
	}
	else
	{
		off = 0;
		mlen = len;
	}
	if ((fd = open(dump, O_RDONLY)) < 0)
		return;

	fd = movefd(fd);
	if (fd == -1)
		return;

	if ((addr = (Wordcode)mmap(NULL, mlen, PROT_READ, MAP_SHARED, fd, off)) ==
	    ((Wordcode)-1))
	{
		close(fd);
		return;
	}
	d = (FuncDump)zalloc(sizeof(*d));
	d->next = dumps;
	dumps = d;
	d->dev = sbuf->st_dev;
	d->ino = sbuf->st_ino;
	d->fd = fd;
#ifdef FD_CLOEXEC
	fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
	d->map = addr + (other ? (len - off) / sizeof(wordcode) : 0);
	d->addr = addr;
	d->len = len;
	d->count = 0;
	d->filename = ztrdup(dump);
}
/* }}} */
/* Code copied from Zshell's parse.c {{{ */
#else

#define custom_zwcstat(f, b) (!!stat(f, b))

/**/
#endif
/* }}} */
/* STATIC FUNCTION: custom_dump_find_func {{{ */
static FDHead
custom_dump_find_func(Wordcode h, char *name)
{
	FDHead n, e = (FDHead)(h + fdheaderlen(h));

	for (n = firstfdhead(h); n < e; n = nextfdhead(n))
		if (!strcmp(name, fdname(n) + fdhtail(n)))
			return n;

	return NULL;
}
/* }}} */
/* STATIC FUNCTION: custom_check_dump_file {{{ */
/**/
static Eprog
custom_check_dump_file(char *file, struct stat *sbuf, char *name, int *ksh,
		       int test_only)
{
	int isrec = 0;
	Wordcode d;
	FDHead h;
	FuncDump f;
	struct stat lsbuf;

	if (!sbuf)
	{
		if (custom_zwcstat(file, &lsbuf))
			return NULL;
		sbuf = &lsbuf;
	}

#ifdef USE_MMAP

rec:

#endif

	d = NULL;

#ifdef USE_MMAP

	for (f = dumps; f; f = f->next)
		if (f->dev == sbuf->st_dev && f->ino == sbuf->st_ino)
		{
			d = f->map;
			break;
		}

#else

	f = NULL;

#endif

	if (!f && (isrec || !(d = custom_load_dump_header(NULL, file, 0))))
		return NULL;

	if ((h = custom_dump_find_func(d, name)))
	{
		/* Found the name. If the file is already mapped, return the eprog,
		 * otherwise map it and just go up. */
		if (test_only)
		{
			/* This is all we need.  Just return dummy. */
			return &dummy_eprog;
		}

#ifdef USE_MMAP

		if (f)
		{
			Eprog prog = (Eprog)zalloc(sizeof(*prog));
			Patprog *pp;
			int np;

			prog->flags = EF_MAP;
			prog->len = h->len;
			prog->npats = np = h->npats;
			prog->nref = 1; /* allocated from permanent storage */
			prog->pats = pp = (Patprog *)zalloc(np * sizeof(Patprog));
			prog->prog = f->map + h->start;
			prog->strs = ((char *)prog->prog) + h->strs;
			prog->shf = NULL;
			prog->dump = f;

			incrdumpcount(f);

			while (np--)
				*pp++ = dummy_patprog1;

			if (ksh)
				*ksh = ((fdhflags(h) & FDHF_KSHLOAD) ? 2 : ((fdhflags(h) & FDHF_ZSHLOAD) ? 0 : 1));

			return prog;
		}
		else if (fdflags(d) & FDF_MAP)
		{
			custom_load_dump_file(file, sbuf, (fdflags(d) & FDF_OTHER), fdother(d));
			isrec = 1;
			goto rec;
		}
		else

#endif

		{
			Eprog prog;
			Patprog *pp;
			int np, fd, po = h->npats * sizeof(Patprog);

			if ((fd = open(file, O_RDONLY)) < 0 ||
			    lseek(fd, ((h->start * sizeof(wordcode)) + ((fdflags(d) & FDF_OTHER) ? fdother(d) : 0)), 0) < 0)
			{
				if (fd >= 0)
					close(fd);
				return NULL;
			}
			d = (Wordcode)zalloc(h->len + po);

			if (read(fd, ((char *)d) + po, h->len) != (int)h->len)
			{
				close(fd);
				zfree(d, h->len);

				return NULL;
			}
			close(fd);

			prog = (Eprog)zalloc(sizeof(*prog));

			prog->flags = EF_REAL;
			prog->len = h->len + po;
			prog->npats = np = h->npats;
			prog->nref = 1; /* allocated from permanent storage */
			prog->pats = pp = (Patprog *)d;
			prog->prog = (Wordcode)(((char *)d) + po);
			prog->strs = ((char *)prog->prog) + h->strs;
			prog->shf = NULL;
			prog->dump = f;

			while (np--)
				*pp++ = dummy_patprog1;

			if (ksh)
				*ksh = ((fdhflags(h) & FDHF_KSHLOAD) ? 2 : ((fdhflags(h) & FDHF_ZSHLOAD) ? 0 : 1));

			return prog;
		}
	}
	return NULL;
}
/* }}} */
/* STATIC FUNCTION: custom_load_dump_header {{{ */
/**/
static Wordcode
custom_load_dump_header(char *nam, char *name, int err)
{
	int fd, v = 1;
	wordcode buf[FD_PRELEN + 1];

	if ((fd = open(name, O_RDONLY)) < 0)
	{
		if (err)
			zwarnnam(nam, "%d: can't open zwc file: %s", __LINE__, name);
		return NULL;
	}
	if (read(fd, buf, (FD_PRELEN + 1) * sizeof(wordcode)) !=
		((FD_PRELEN + 1) * sizeof(wordcode)) ||
	    (v = (fdmagic(buf) != FD_MAGIC && fdmagic(buf) != FD_OMAGIC)) ||
	    strcmp(fdversion(buf), getsparam("ZSH_VERSION")))
	{
		if (err)
		{
			if (!v)
			{
				zwarnnam(nam, "%d: zwc file has wrong version (zsh-%s): %s",
					 __LINE__, fdversion(buf), name);
			}
			else
				zwarnnam(nam, "%d: invalid zwc file: %s", __LINE__, name);
		}
		close(fd);
		return NULL;
	}
	else
	{
		int len;
		Wordcode head;

		if (fdmagic(buf) == FD_MAGIC)
		{
			len = fdheaderlen(buf) * sizeof(wordcode);
			head = (Wordcode)zhalloc(len);
		}
		else
		{
			int o = fdother(buf);

			if (lseek(fd, o, 0) == -1 ||
			    read(fd, buf, (FD_PRELEN + 1) * sizeof(wordcode)) !=
				((FD_PRELEN + 1) * sizeof(wordcode)))
			{
				zwarnnam(nam, "%d: invalid zwc file: %s", __LINE__, name);
				close(fd);
				return NULL;
			}
			len = fdheaderlen(buf) * sizeof(wordcode);
			head = (Wordcode)zhalloc(len);
		}
		memcpy(head, buf, (FD_PRELEN + 1) * sizeof(wordcode));

		len -= (FD_PRELEN + 1) * sizeof(wordcode);
		if (read(fd, head + (FD_PRELEN + 1), len) != len)
		{
			close(fd);
			zwarnnam(nam, "%d: invalid zwc file: %s", __LINE__, name);
			return NULL;
		}
		close(fd);
		return head;
	}
}
/* }}} */

/*
 * Main builtin `zpmod' and its subcommands
 */

/* FUNCTION: bin_zpmod {{{ */
static int
bin_zpmod(char *nam, char **argv, UNUSED(Options ops), UNUSED(int func))
{
	char *subcmd = NULL;
	int ret = 0;

	if (OPT_ISSET(ops, 'h'))
	{
		zpmod_usage();
		return 0;
	}

	if (!*argv)
	{
		zwarnnam(nam, "%d: `zpmod' takes a sub-command as first argument, see -h", __LINE__);
		return 1;
	}

	subcmd = *argv++;

	if (0 == strcmp(subcmd, "report-append"))
	{
		char *target = NULL, *body = NULL;
		int target_len = 0, body_len = 0;

		target = *argv++;
		if (!target)
		{
			zwarnnam(nam, "%d: `report-append' is missing the target plugin ID (like \"z-shell/zbrowse\", see -h", __LINE__);
			return 1;
		}
		target = zp_unmetafy_zalloc(target, &target_len);
		if (!target)
		{
			zwarnnam(nam, "%d: Couldn't allocate new memory (1), operation aborted", __LINE__);
			return 1;
		}

		body = *argv++;
		if (!body)
		{
			zwarnnam(nam, "%d: `report-append' is missing the report-body to append, see -h", __LINE__);
			return 1;
		}
		body_len = strlen(body);

		ret = zp_append_report(nam, target, target_len, body, body_len);
		zfree(target, target_len);
	}
	else if (0 == strcmp(subcmd, "source-study"))
	{
		char *report;
		int rep_size;
		report = zp_build_source_report(!zp_has_option(argv, 'l'), &rep_size);
		fprintf(stdout, "%s", report ? report : "Unknown error, aborted");
		fflush(stdout);
		if (rep_size)
		{
			zfree(report, rep_size);
		}
		else if (report)
		{
			zsfree(report);
		}
	}
	else if (0 == strcmp(subcmd, "clear-path-cache"))
	{
		if (zp_path_cache) {
			zp_path_cache_clear(zp_path_cache);
			fprintf(stdout, "Path cache cleared (%d entries removed)\n", zp_path_cache->count);
			fflush(stdout);
		} else {
			fprintf(stdout, "Path cache not initialized\n");
			fflush(stdout);
		}
	}
	else if (0 == strcmp(subcmd, "compile-config"))
	{
		if (!zp_compile_config) {
			fprintf(stdout, "Compilation configuration not initialized\n");
			fflush(stdout);
			return 1;
		}

		char *action = *argv++;
		if (!action) {
			/* Display current config */
			fprintf(stdout, "Compilation Configuration:\n");
			fprintf(stdout, "  Enabled: %s\n", zp_compile_config->enabled ? "yes" : "no");
			fprintf(stdout, "  Debug Mode: %s\n", zp_compile_config->debug_mode ? "yes" : "no");
			fprintf(stdout, "  Batch Mode: %s\n", zp_compile_config->batch_mode ? "yes" : "no");
			fprintf(stdout, "  Batch Size: %d\n", zp_compile_config->batch_size);
			fprintf(stdout, "  Batch Interval: %d seconds\n", zp_compile_config->batch_interval);
			fprintf(stdout, "  Max File Size: %d bytes\n", zp_compile_config->max_file_size);

			fprintf(stdout, "  Exclusion Patterns: %d\n", zp_compile_config->exclusion_count);
			for (int i = 0; i < zp_compile_config->exclusion_count; i++) {
				fprintf(stdout, "    %s\n", zp_compile_config->exclusion_patterns[i]);
			}

			fprintf(stdout, "  Inclusion Patterns: %d\n", zp_compile_config->inclusion_count);
			for (int i = 0; i < zp_compile_config->inclusion_count; i++) {
				fprintf(stdout, "    %s\n", zp_compile_config->inclusion_patterns[i]);
			}

			fprintf(stdout, "  Pending Files: %d\n", zp_compile_config->pending_count);
			fflush(stdout);
		} else if (0 == strcmp(action, "enable")) {
			zp_compile_config->enabled = 1;
			fprintf(stdout, "Compilation enabled\n");
			fflush(stdout);
		} else if (0 == strcmp(action, "disable")) {
			zp_compile_config->enabled = 0;
			fprintf(stdout, "Compilation disabled\n");
			fflush(stdout);
		} else if (0 == strcmp(action, "batch")) {
			char *mode = *argv++;
			if (!mode) {
				fprintf(stdout, "Batch mode is %s\n",
					zp_compile_config->batch_mode ? "enabled" : "disabled");
				fflush(stdout);
			} else if (0 == strcmp(mode, "on")) {
				zp_compile_config->batch_mode = 1;
				fprintf(stdout, "Batch mode enabled\n");
				fflush(stdout);
			} else if (0 == strcmp(mode, "off")) {
				zp_compile_config->batch_mode = 0;
				fprintf(stdout, "Batch mode disabled\n");
				fflush(stdout);
			} else {
				fprintf(stdout, "Invalid batch mode: use 'on' or 'off'\n");
				fflush(stdout);
				return 1;
			}
		} else if (0 == strcmp(action, "exclude")) {
			char *pattern = *argv++;
			if (!pattern) {
				fprintf(stdout, "Missing pattern to exclude\n");
				fflush(stdout);
				return 1;
			}

			if (zp_compile_config_add_exclusion(zp_compile_config, pattern)) {
				fprintf(stdout, "Failed to add exclusion pattern: %s\n", pattern);
				fflush(stdout);
				return 1;
			}

			fprintf(stdout, "Added exclusion pattern: %s\n", pattern);
			fflush(stdout);
		} else if (0 == strcmp(action, "include")) {
			char *pattern = *argv++;
			if (!pattern) {
				fprintf(stdout, "Missing pattern to include\n");
				fflush(stdout);
				return 1;
			}

			if (zp_compile_config_add_inclusion(zp_compile_config, pattern)) {
				fprintf(stdout, "Failed to add inclusion pattern: %s\n", pattern);
				fflush(stdout);
				return 1;
			}

			fprintf(stdout, "Added inclusion pattern: %s\n", pattern);
			fflush(stdout);
		} else if (0 == strcmp(action, "process-batch")) {
			zp_compile_config_process_batch(zp_compile_config);
			fprintf(stdout, "Processed pending compilation batch\n");
			fflush(stdout);
		} else {
			fprintf(stdout, "Unknown compile-config action: %s\n", action);
			fflush(stdout);
			return 1;
		}
	}
	else if (0 == strcmp(subcmd, "lazy-load"))
	{
		if (!zp_lazy_loader) {
			fprintf(stdout, "Lazy loading system not initialized\n");
			fflush(stdout);
			return 1;
		}

		char *action = *argv++;
		if (!action) {
			/* Display status */
			fprintf(stdout, "Lazy Loading Status:\n");
			fprintf(stdout, "  Debug Mode: %s\n", zp_lazy_loader->debug_mode ? "yes" : "no");
			fprintf(stdout, "  Registered Functions: %d\n", zp_lazy_loader->function_count);

			for (int i = 0; i < zp_lazy_loader->function_count; i++) {
				ZpLazyFunction lazy_func = zp_lazy_loader->functions[i];
				fprintf(stdout, "    %s: %s (%s)\n",
					lazy_func->name,
					lazy_func->loaded ? "loaded" : "not loaded",
					lazy_func->library_path);
			}
			fflush(stdout);
		} else if (0 == strcmp(action, "debug")) {
			char *mode = *argv++;
			if (!mode) {
				fprintf(stdout, "Debug mode is %s\n",
					zp_lazy_loader->debug_mode ? "enabled" : "disabled");
				fflush(stdout);
			} else if (0 == strcmp(mode, "on")) {
				zp_lazy_loader_set_debug(zp_lazy_loader, 1);
				fprintf(stdout, "Debug mode enabled\n");
				fflush(stdout);
			} else if (0 == strcmp(mode, "off")) {
				zp_lazy_loader_set_debug(zp_lazy_loader, 0);
				fprintf(stdout, "Debug mode disabled\n");
				fflush(stdout);
			} else {
				fprintf(stdout, "Invalid debug mode: use 'on' or 'off'\n");
				fflush(stdout);
				return 1;
			}
		} else if (0 == strcmp(action, "register")) {
			char *name = *argv++;
			char *library = *argv++;

			if (!name || !library) {
				fprintf(stdout, "Usage: zpmod lazy-load register {function-name} {library-path}\n");
				fflush(stdout);
				return 1;
			}

			if (zp_lazy_loader_register(zp_lazy_loader, name, library)) {
				fprintf(stdout, "Failed to register function: %s\n", name);
				fflush(stdout);
				return 1;
			}

			fprintf(stdout, "Registered function %s from %s\n", name, library);
			fflush(stdout);
		} else if (0 == strcmp(action, "unload")) {
			zp_lazy_loader_unload_all(zp_lazy_loader);
			fprintf(stdout, "All functions unloaded\n");
			fflush(stdout);
		} else {
			fprintf(stdout, "Unknown lazy-load action: %s\n", action);
			fflush(stdout);
			return 1;
		}
	}
	else
	{
		zwarnnam(nam, "%d: Unknown zpmod-module command: `%s', see `-h'", __LINE__, subcmd);
	}

	return ret;
}
/* }}} */
/* FUNCTION: zpmod_usage {{{ */
/**/
void zpmod_usage()
{
	fprintf(stdout, "Usage: zpmod {subcommand} {subcommand-arguments}\n"
			"       zpmod report-append {plugin-ID} {new-report-body}\n"
			"       zpmod source-study [-l]\n"
			"       zpmod clear-path-cache\n"
			"       zpmod compile-config [action] [arguments]\n"
			"       zpmod lazy-load [action] [arguments]\n"
			"\n"
			"[33mCommand <report-append>:[0m\n"
			"\n"
			"Used by zpmod internally to speed up loading plugins with tracking (reporting).\n"
			"It extends the given field {plugin-ID} in $ZI_REPORTS hash, with the given string\n"
			"{new-report-body}.\n"
			"\n"
			"[33mCommand <source-study>:[0m\n"
			"\n"
			"Displays list of files loaded via `source' or `.' builtins, with duration that each\n"
			"loading lasted, in milliseconds. The module tracks all calls to those builtins and\n"
			"measures the time each call took. This can be used to e.g. profile loading of plugins,\n"
			"regardless of the plugin manager used.\n"
			"\n"
			"Option -l shows full paths to the files.\n"
			"\n"
			"[33mCommand <clear-path-cache>:[0m\n"
			"\n"
			"Clears the internal cache of file paths. The module maintains a cache of frequently\n"
			"checked file paths to improve performance when loading files. This command can be\n"
			"used to reset the cache if needed during development or troubleshooting.\n"
			"\n"
			"[33mCommand <compile-config>:[0m\n"
			"\n"
			"Manages the automatic compilation system configuration. When called without arguments,\n"
			"it displays the current configuration. The following actions are supported:\n"
			"\n"
			"  [none]               - Display current configuration\n"
			"  enable               - Enable automatic compilation\n"
			"  disable              - Disable automatic compilation\n"
			"  batch [on|off]       - Enable/disable batch mode or show current state\n"
			"  exclude {pattern}    - Add a pattern to exclude from compilation\n"
			"  include {pattern}    - Add a pattern to always include for compilation\n"
			"  process-batch        - Force processing of the pending compilation batch\n"
			"\n"
			"Batch mode queues files for compilation and processes them in batches to reduce\n"
			"overhead. Patterns use extended regular expressions for matching file paths.\n"
			"\n"
			"[33mCommand <lazy-load>:[0m\n"
			"\n"
			"Manages the lazy loading system for dynamically loading functions on demand. When\n"
			"called without arguments, it displays the current status of registered functions.\n"
			"The following actions are supported:\n"
			"\n"
			"  [none]               - Display current status of all registered functions\n"
			"  debug [on|off]       - Enable/disable debug mode or show current state\n"
			"  register {name} {lib}- Register a function for lazy loading from a library\n"
			"  unload               - Unload all loaded functions to free memory\n"
			"\n"
			"Lazy loading improves performance by only loading rarely used functionality when\n"
			"it's actually needed, reducing memory usage and startup time.\n");
	fflush(stdout);
}
/* }}} */

/* FUNCTION: zp_append_report {{{ */
/**/
static int
zp_append_report(const char *nam, const char *target, int target_len, const char *body, int body_len)
{
	Param pm = NULL, val_pm = NULL;
	HashTable ht = NULL;
	HashNode hn = NULL;
	char *target_string = NULL;
	int target_string_len = 0, new_extended_len = 0;

	/* Get ZI_REPORTS associative array */
	pm = (Param)paramtab->getnode(paramtab, "ZI_REPORTS");
	if (!pm)
	{
		zwarnnam(nam, "%d: Parameter $ZI_REPORTS isn't declared. zpmod is not loaded? I.e. not sourced.", __LINE__);
		return 1;
	}

	/* Get ZI_REPORTS[{target}] hashed Param */
	ht = pm->u.hash;
	hn = gethashnode2(ht, target);
	val_pm = (Param)hn;
	if (!val_pm)
	{
		zwarnnam(nam, "%d: Plugin %s isn't registered, cannot append to its report.", __LINE__, target);
		return 1;
	}

	/* Nothing to append? */
	if (body_len == 0)
	{
		return 0;
	}

	/* Get string that the hashed Param holds */
	target_string = val_pm->u.str;
	if (!target_string)
	{
		target_string_len = 0;
	}
	else
	{
		target_string_len = strlen(target_string);
	}

	/* Extend the string with additional body_len-bytes */
	new_extended_len = target_string_len + body_len;
	target_string = realloc(target_string, (new_extended_len + 1) * sizeof(char));
	if (NULL == target_string)
	{
		zwarnnam(nam, "%d: Couldn't allocate new memory (2), operation aborted", __LINE__);
		return 1;
	}

	/* Copy contents of body, null terminate */
	memcpy(target_string + target_string_len, body, sizeof(char) * body_len);
	target_string[new_extended_len] = '\0';

	/* Store the pointer in case realloc() allocated a new buffer */
	val_pm->u.str = target_string;

	return 0;
}
/* }}} */
/* FUNCTION: zp_build_source_report {{{ */
/**/
char *zp_build_source_report(int no_paths, int *rep_size)
{
	char *report, zp_tmp[20];
	int current_size, space_left, current_end, idx, printed;
	SEventNode node;
	FILE *null_fle;

	current_size = 127;
	current_end = 0;
	report = (char *)zalloc(sizeof(char) * (current_size + 1));
	space_left = 127;
	report[current_end] = '\0';
	*rep_size = current_size + 1;

	if (!report)
	{
		*rep_size = 0;
		return ztrdup("ERROR: couldn't allocate initial buffer, aborted\n");
	}

	null_fle = fopen("/dev/null", "w");
	if (!null_fle)
	{
		zfree(report, *rep_size);
		*rep_size = 0;
		return ztrdup("ERROR: couldn't open /dev/null, aborted\n");
	}

	for (idx = 1; idx <= zp_sevent_count; ++idx)
	{
		sprintf(zp_tmp, "%d", idx);
		zp_tmp[19] = '\0';

		if (!(node = (SEventNode)gethashnode2(zp_source_events, zp_tmp)))
		{
			continue;
		}

		printed = fprintf(null_fle, "%4.0lf ms    %s\n", node->event.duration,
				  no_paths ? node->event.file_name : node->event.full_path);
		if (space_left < printed)
		{
			char *report_;
			current_size += printed - space_left + 25;
			space_left += printed - space_left + 25;
			report_ = zrealloc(report, sizeof(char) * (current_size + 1));
			if (!report_)
			{
				zfree(report, *rep_size);
				*rep_size = 0;
				fclose(null_fle);
				return ztrdup("ERROR: Couldn't realloc buffer, aborted\n");
			}
			report = report_;
			*rep_size = current_size + 1;
		}

		printed = sprintf(report + current_end, "%4.0lf ms    %s\n", node->event.duration,
				  no_paths ? node->event.file_name : node->event.full_path);
		current_end += printed;
		space_left -= printed;
	}
	fclose(null_fle);
	return report;
}
/* }}} */
/*
 * Needed tool-functions, like function creating a hash parameter
 */

/* FUNCTION: zp_createhashtable {{{ */
/**/
static HashTable
zp_createhashtable(char *name)
{
	HashTable ht;

	ht = newhashtable(8, name, NULL);

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
/* }}} */
/* FUNCTION: zp_free_sevent_node {{{ */
/**/
static void
zp_free_sevent_node(HashNode hn)
{
	zsfree(hn->nam);
	zfree(hn, sizeof(struct zp_sevent_node));
}
/* }}} */
/* FUNCTION: zp_freeparamnode {{{ */
/**/
void zp_freeparamnode(HashNode hn)
{
	Param pm = (Param)hn;

	/* Upstream: The second argument of unsetfn() is used by modules to
	 * differentiate "exp"licit unset from implicit unset, as when
	 * a parameter is going out of scope.  It's not clear which
	 * of these applies here, but passing 1 has always worked.
	 */

	/* if (delunset) */
	pm->gsu.s->unsetfn(pm, 1);

	zsfree(pm->node.nam);
	/* If this variable was tied by the user, ename was ztrdup'd */
	if (pm->node.flags & PM_TIED && pm->ename)
	{
		zsfree(pm->ename);
		pm->ename = NULL;
	}
	zfree(pm, sizeof(struct param));
}
/* }}} */

/*
 * Tool-functions that are more hacky or problem-solving
 */

/* FUNCTION: zp_has_option {{{ */
/**/
static int
zp_has_option(char **argv, char opt)
{
	char *string;
	while ((string = *argv))
	{
		if (string[0] == '-')
		{
			while (*++string)
			{
				if (string[0] == opt)
				{
					return 1;
				}
			}
		}
		++argv;
	}
	return 0;
}
/* }}} */
/* FUNCTION: my_ztrdup_glen {{{ */
/**/
char *
my_ztrdup_glen(const char *s, unsigned *len_ret)
{
	char *t;

	if (!s)
		return NULL;
	t = (char *)zalloc((*len_ret = strlen((char *)s)) + 1);
	strcpy(t, s);
	return t;
}
/* }}} */
/* FUNCTION: zp_unmetafy_zalloc {{{ */
/*
 * Unmetafy that:
 * - duplicates buffer to work on it - original buffer is unchanged, can be zsfree'd,
 * - does zalloc of exact size for the new unmeta-string - this string can be zfree'd,
 * - restores work-buffer to original meta-content, to restore strlen - thus work-buffer can be zsfree'd,
 * - returns actual length of the output unmeta-string, which should be passed to zfree.
 *
 * This function can be avoided if there's no need for new buffer, user should first strlen
 * the metafied string, store the length into a variable (e.g. meta_length), then unmetafy,
 * use the unmeta-content, then zfree( buf, meta_length ).
 */

/**/
char *
zp_unmetafy_zalloc(const char *to_copy, int *new_len)
{
	char *work, *to_return;
	int my_new_len = 0;
	unsigned meta_length = 0;

	work = my_ztrdup_glen(to_copy, &meta_length);
	if (!work)
	{
		return NULL;
	}

	work = unmetafy(work, &my_new_len);

	if (new_len)
		*new_len = my_new_len;

	to_return = (char *)zalloc((my_new_len + 1) * sizeof(char));
	if (!to_return)
	{
		zfree(work, meta_length);
		return NULL;
	}

	memcpy(to_return, work, sizeof(char) * my_new_len); /* memcpy handles $'\0' */
	to_return[my_new_len] = '\0';

	/* Restore original content and correctly zsfree(). */
	/* UPDATE: instead of zsfree() here now it is
	 * zfree() that's used and the length it needs
	 * is taken above from my_ztrdup_glen */
	zfree(work, meta_length);

	return to_return;
}
/* }}} */

/*
 * Zshell module architecture data structures
 */

/* ARRAY: struct builtin bintab[] {{{ */
static struct builtin bintab[] =
    {
	BUILTIN("custom_dot", 0, bin_custom_dot, 1, -1, 0, NULL, NULL),
	BUILTIN("zpmod", 0, bin_zpmod, 0, -1, 0, "h", NULL),
};
/* }}} */
/* STRUCT: struct features module_features {{{ */
static struct features module_features =
    {
	bintab, sizeof(bintab) / sizeof(*bintab),
	NULL, 0,
	NULL, 0,
	NULL, 0,
	0};
/* }}} */

/*
 * Zshell module architecture functions
 */

/* FUNCTION: setup_ {{{ */
/**/
int setup_(UNUSED(Module m))
{
	zp_setup_options_table();
	Builtin bn = (Builtin)builtintab->getnode2(builtintab, ".");
	originalDot = bn->handlerfunc;
	bn->handlerfunc = bin_custom_dot;

	bn = (Builtin)builtintab->getnode2(builtintab, "source");
	originalSource = bn->handlerfunc;
	bn->handlerfunc = bin_custom_dot;

	/* Create private hash with source_prepare requests */
	if (!(zp_source_events = zp_createhashtable("zp_source_events")))
	{
		zwarn("Cannot create the hash table");
		return 1;
	}

	/* Initialize path cache */
	zp_path_cache = zp_path_cache_init(ZP_CACHE_SIZE, ZP_CACHE_LIFETIME);
	if (!zp_path_cache) {
		zwarn("Could not initialize path cache");
	}

	/* Initialize compilation configuration */
	zp_compile_config = zp_compile_config_init();
	if (!zp_compile_config) {
		zwarn("Could not initialize compilation configuration");
	} else {
		/* Load settings from environment variables */
		zp_compile_config_load_env(zp_compile_config);
	}

	/* Initialize lazy loading system */
	zp_lazy_loader = zp_lazy_loader_init();
	if (!zp_lazy_loader) {
		zwarn("Could not initialize lazy loading system");
	}

	return 0;
}
/* }}} */
/* FUNCTION: features_ {{{ */
/**/
int features_(Module m, char ***features)
{
	*features = featuresarray(m, &module_features);
	return 0;
}
/* }}} */
/* FUNCTION: enables_ {{{ */
/**/
int enables_(Module m, int **enables)
{
	return handlefeatures(m, &module_features, enables);
}
/* }}} */
/* FUNCTION: boot_ {{{ */
/**/
int boot_(Module m)
{
	return 0;
}
/* }}} */
/* FUNCTION: cleanup_ {{{ */
/**/
int cleanup_(Module m)
{
	return setfeatureenables(m, &module_features, NULL);
}
/* }}} */
/* FUNCTION: finish_ {{{ */
/**/
int finish_(UNUSED(Module m))
{
	Builtin bn = (Builtin)builtintab->getnode2(builtintab, ".");
	bn->handlerfunc = originalDot;

	bn = (Builtin)builtintab->getnode2(builtintab, "source");
	bn->handlerfunc = originalSource;

	/* Destroy path cache */
	if (zp_path_cache) {
		zp_path_cache_destroy(zp_path_cache);
		zp_path_cache = NULL;
	}

	/* Destroy compilation configuration */
	if (zp_compile_config) {
		zp_compile_config_destroy(zp_compile_config);
		zp_compile_config = NULL;
	}

	/* Destroy lazy loader */
	if (zp_lazy_loader) {
		zp_lazy_loader_destroy(zp_lazy_loader);
		zp_lazy_loader = NULL;
	}

	printf("zi/zpmod module unloaded\n");
	fflush(stdout);
	return 0;
}
/* }}} */
