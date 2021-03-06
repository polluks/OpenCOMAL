/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

/*
 * Main file of OpenComal 
 */

#define _XOPEN_SOURCE 700

#define EXTERN

#include "pdcglob.h"
#include "pdcenv.h"
#include "pdccloop.h"
#include "pdcexp.h"
#include "pdcmisc.h"
#include "pdcstr.h"
#include "long.h"
#include "msgnrs.h"
#include "version.h"

#include <string.h>
#include <unistd.h>
#include <locale.h>

#include "except.h"
#include "fmt.h"

#define LOCNAME_MAX 32
#define ENVVAL_MAX 32

extern int      yydebug;

#ifdef TURBOC
unsigned        _stklen = 32768;
#endif

PRIVATE const char *
safe_getenv(const char *name)
{
    const char     *value;

    if ((value = getenv(name)) == NULL) {
        return "(unset)";
    } else {
        static char     result[ENVVAL_MAX];

        Fmt_sfmt(result, ENVVAL_MAX, "\"%s\"", value);
        return result;
    }
}

PRIVATE const char *
safe_setlocale(void)
{
    const char     *result;

    if ((result = setlocale(LC_ALL, "")) == NULL) {
        perror("setlocale");
        Fmt_print("warning: Setting locale failed.\n"
                  "warning: Please check that your locale settings:\n"
                  "\tLC_ALL = %s,\n"
                  "\tLANG = %s\n"
                  "    are supported and installed on your system.\n"
                  "warning: Falling back to the standard locale (\"C\").\n",
                  safe_getenv("LC_ALL"), safe_getenv("LANG"));
        if (setlocale(LC_ALL, "C") == NULL) {
            perror("setlocale");
            exit(EXIT_FAILURE);
        }
        result = NULL;
    }
    return result;
}

PRIVATE         locale_t
safe_newlocale(const char *nlocname, locale_t loc, const char *fallback)
{
    locale_t        result;

    result = newlocale(LC_ALL_MASK, nlocname, loc);
    // Note that this does *not* create a new locale object.
    // It modifies 'loc' instead.

    if (result == (locale_t) 0) {
        perror("newlocale");
        Fmt_print(str_ltou(catgets(catdesc, MainSet, MainNewlocaleFailed,
                                   "warning: Setting locale failed.\n"
                                   "warning: Please check that the locale \"%s\" is supported and installed on your system.\n"
                                   "warning: Falling back to the global locale (\"%s\").\n")),
                  nlocname, fallback);
        result = loc;
    }
    return result;
}

PUBLIC int
main(int argc, char *argv[])
{
    int             c;
    bool            errflg = false;
    const char     *locname;

    TRY             Fmt_register('D', cvt_D);
    locname = safe_setlocale();

    catdesc = catopen("opencomal.cat", NL_CAT_LOCALE);

    // Need this now, because we'll have to convert error messages before
    // curses initialisation.
    latin_to_utf8 = iconv_open("utf8", "latin-9");

    while ((c = getopt(argc, argv, ":dym:")) != -1) {
        switch (c) {
        case 'd':
            comal_debug = true;
            break;
        case 'y':
            yydebug++;
            break;
        case 'm':
            catclose(catdesc);
            catdesc = catopen(optarg, NL_CAT_LOCALE);
            if (catdesc == (nl_catd) - 1) {
                perror("catopen");
            }
            break;
        case ':':              /* -m without operand */
            Fmt_fprint(stderr,
                       str_ltou(catgets
                                (catdesc, MainSet, MainReqOp,
                                 "Option -%c requires an operand\n")),
                       optopt);
            errflg = true;
            break;
        case '?':
            Fmt_fprint(stderr,
                       str_ltou(catgets
                                (catdesc, MainSet, MainBadOpt,
                                 "Unrecognised option: '-%c'\n")), optopt);
            errflg = true;
            break;
        default:
            errflg = true;
            break;
        }
    }
    if (errflg) {
        Fmt_fprint(stderr,
                   str_ltou(catgets
                            (catdesc, MainSet, MainUsage,
                             "usage: %s [-dy] [-m <msg-catalog>] ...\n")),
                   argv[0]);
        RETURN          EXIT_FAILURE;
    }

    latin_loc = duplocale(LC_GLOBAL_LOCALE);
    if (latin_loc == (locale_t) 0) {
        perror("duplocale");
        RETURN          EXIT_FAILURE;
    }
    if (locname != NULL) {
        int             utf8_suffixlen,
                        lang_countrylen;
#ifdef __APPLE__
        const char     *utf8_suffix = ".UTF-8";
#else
        const char     *utf8_suffix = ".utf8";
#endif

        utf8_suffixlen = strlen(utf8_suffix);
        lang_countrylen = strlen(locname) - utf8_suffixlen;
        if (strncmp(locname + lang_countrylen, utf8_suffix, utf8_suffixlen)
            == 0) {
            char            nlocname[LOCNAME_MAX];
#ifdef __APPLE__
            const char     *latin_suffix = ".ISO8859-15";
#else
            const char     *latin_suffix = "@euro";
#endif

            term_strncpy(nlocname, locname, lang_countrylen + 1);
            strncat(nlocname, latin_suffix,
                    LOCNAME_MAX - lang_countrylen - 1);
            latin_loc = safe_newlocale(nlocname, latin_loc, locname);
        }
    }

    sys_init();

    my_printf(MSG_DIALOG, true,
              catgets(catdesc, MainSet, MainBanner,
                      "OpenComal -- A free Comal implementation (version %s; %s; build %s)"),
              OPENCOMAL_VERSION, HOST_OS, OPENCOMAL_BUILD);

    my_printf(MSG_DIALOG, true, "%s",
              catgets(catdesc, MainSet, MainCopyright,
                      "             (c) Copyright 1992-2002  Jos Visser <josv@osp.nl>"));
    my_printf(MSG_DIALOG, true, "%s%s",
              catgets(catdesc, MainSet, MainBuilt,
                      "             Last modified on "), BUILD_DATE);
    my_nl(MSG_DIALOG);
    my_printf(MSG_DIALOG, true, "%s",
              catgets(catdesc, MainSet, MainLic1,
                      "OpenComal is licensed under the GNU General Public License (GPL) version 3"));
    my_printf(MSG_DIALOG, true, "%s",
              catgets(catdesc, MainSet, MainLic2,
                      "(The GPL contains a very nice statement on WARRANTY; you might want to read it)"));
    my_nl(MSG_DIALOG);

    mem_init();

    runfilename = NULL;
    curenv = env_new("nirvana");
    entering = 0;
    sel_infile = NULL;
    sel_outfile = NULL;

    pdc_go(argc - optind + 1, &(argv[optind - 1]));

    if (setjmp(RESTART) == 0) {
        if (sel_infile != NULL && fclose(sel_infile) == EOF) {
            perror("fclose");
        }
        if (sel_outfile != NULL && fclose(sel_outfile) == EOF) {
            perror("fclose");
        }
        clear_env(curenv);
        mem_tini();
    }

    sys_tini();
    freelocale(latin_loc);
    catclose(catdesc);
    ELSE            Fmt_fprint(stderr,
                               "An internal error has occurred from which there is "
                               "no recovery.\nPlease create a new issue at "
                               "github.com/poldy/OpenCOMAL .\nNote the "
                               "following message, which will help the maintainers\n"
                               "find the cause of this error.\n\n");
    RERAISE;
    END_TRY;
    return EXIT_SUCCESS;
}
