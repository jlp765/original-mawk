/********************************************
version.c
copyright 2008-2014,2015.  Thomas E. Dickey
copyright 1991-1996,2014   Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/*
 * $MawkId: version.c,v 1.21 2015/05/02 12:01:12 tom Exp $
 */

#include "mawk.h"
#include "init.h"
#include "patchlev.h"

#define	 VERSION_STRING	 \
  "mawk %d.%d%s %s\n\
Copyright 2008-2014,2015, Thomas E. Dickey\n\
Copyright 1991-1996,2014, Michael D. Brennan\n\n"

#define FMT_N "%-20s%.0f\n"
#define FMT_S "%-20s%s\n"

/* print VERSION and exit */
void
print_version(void)
{
    printf(VERSION_STRING, PATCH_BASE, PATCH_LEVEL, PATCH_STRING, DATE_STRING);
    fflush(stdout);

#define SHOW_RANDOM "random-funcs:"
#if defined(NAME_RANDOM)
    fprintf(stderr, FMT_S, SHOW_RANDOM, NAME_RANDOM);
#else
    fprintf(stderr, FMT_S, SHOW_RANDOM, "internal");
#endif

#define SHOW_REGEXP "regex-funcs:"
#ifdef LOCAL_REGEXP
    fprintf(stderr, FMT_S, SHOW_REGEXP, "internal");
#else
    fprintf(stderr, FMT_S, SHOW_REGEXP, "external");
#endif

    fprintf(stderr, "\ncompiled limits:\n");
    fprintf(stderr, FMT_N, "sprintf buffer", (double) SPRINTF_LIMIT);
    fprintf(stderr, FMT_N, "maximum-integer", (double) MAX__INT);
#if 0
    /* we could show these, but for less benefit: */
    fprintf(stderr, FMT_N, "maximum-unsigned", (double) MAX__UINT);
    fprintf(stderr, FMT_N, "maximum-long", (double) MAX__LONG);
#endif

    fprintf(stderr, "\ncompiled extras:\n");
#define SHOW_MATHC99 "C99 math-funcs:"
#ifdef HAVE_C99_FUNCS
    fprintf(stderr, FMT_S, SHOW_MATHC99, "enabled");
 #define SHOW_GNUMATH "GNU Bessel-funcs:"
 #ifdef _GNU_SOURCE
        fprintf(stderr, FMT_S, SHOW_GNUMATH, "enabled");
 #else
        fprintf(stderr, FMT_S, SHOW_GNUMATH, "disabled");
 #endif
#endif
#ifdef REPETITIONS
    fprintf(stderr, FMT_S, "RE intervals: ", "enabled");
#else
    fprintf(stderr, FMT_S, "RE intervals: ", "enabled by -W repetitions");
#endif
    mawk_exit(0);
}
