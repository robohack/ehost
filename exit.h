/*
** Various exit codes.
**
**	They come from <sysexits.h>
**	Defined here to avoid including /usr/ucbinclude on solaris 2.x
**
**	@(#)exit.h              e07@nikhef.nl (Eric Wassenaar) 961010
*/

#ident "@(#)host:$Name:  $:$Id: exit.h,v 1.3 2003-03-28 21:57:23 -0800 woods Exp $"

#ifndef HAVE_SYSEXITS_H

# undef EX_OK				/* remove EX_OK defined in SysVR4.2 */
# define EX_OK		0		/* successful termination */

/*
 * use the traditional default values for exit codes unless somebody want's to
 * override them for some weird reason
 */
# ifndef EX__BASE
#  define EX__BASE	64
# endif

# define EX_USAGE	(EX__BASE+0)	/* command line usage error */
# define EX_DATAERR	(EX__BASE+1)	/* data format error */
# define EX_NOINPUT	(EX__BASE+2)	/* cannot open input */
# define EX_NOUSER	(EX__BASE+3)	/* addressee unknown */
# define EX_NOHOST	(EX__BASE+4)	/* host name unknown */
# define EX_UNAVAILABLE	(EX__BASE+5)	/* service unavailable */
# define EX_SOFTWARE	(EX__BASE+6)	/* internal software error */
# define EX_OSERR	(EX__BASE+7)	/* system error */
# define EX_OSFILE	(EX__BASE+8)	/* critical OS file missing */
# define EX_CANTCREAT	(EX__BASE+9)	/* can't create (user) output file */
# define EX_IOERR	(EX__BASE+10)	/* error in file i/o */
# define EX_TEMPFAIL	(EX__BASE+11)	/* temp failure; user can retry */
# define EX_PROTOCOL	(EX__BASE+12)	/* remote error in protocol */
# define EX_NOPERM	(EX__BASE+13)	/* permission denied */
# define EX_CONFIG	(EX__BASE+14)	/* configuration error */
# define EX_NOTFOUND	(EX__BASE+15)	/* entry not found */

# undef EX__MAX
# define EX__MAX	(EX__BASE+15)

#else	/* HAVE_SYSEXITS_H */

# include <sysexits.h>

/*
 * sendmail's <sysexits.h> rev 4.3 from 12/15/87 has neither EX_CONFIG nor EX__MAX
 */
#ifndef EX_CONFIG
# define EX_CONFIG	(EX__BASE+14)	/* configuration error */
# define EX__MAX	(EX__BASE+14)
#else  /* EX_CONFIG */
/*
 * sendmail's <sysexits.h> rev 4.5 from 7/6/88 has no EX__MAX but does of
 * course have EX_CONFIG
 *
 * sendmail's <sysexits.h> rev 4.8 from 4/3/91 has EX__BASE and EX__MAX
 */
# ifndef EX__MAX
#  if (EX_CONFIG != (EX__BASE+14))
#   include "ERROR:  your <sysexits.h> is too non-standard -- remove HAVE_SYSEXITS!"
#  endif
#  define EX__MAX	(EX__BASE+14)
# endif
#endif /* EX_CONFIG */

/*
 * This one still isn't in sendmail 8.12.8's copy, so watch out....
 */
# ifndef EX_NOTFOUND
#  define EX_NOTFOUND	(EX__BASE+15)
#  ifdef EX__MAX
#   if (EX__MAX != (EX__BASE+14))
#    include "ERROR: clash with some system EX_ value at (EX__BASE+14)!"
#   endif
#   undef EX__MAX
#  endif
#  define EX__MAX	(EX__BASE+15)
# endif

#endif	/* HAVE_SYSEXITS */
