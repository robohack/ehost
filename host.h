/*
** Master include file of the host utility.
**
** from: @(#)host.h              e07@nikhef.nl (Eric Wassenaar) 991529
*/

#ident "@(#)host:$Name:  $:$Id: host.h,v 1.8 2003-03-30 20:50:26 -0800 woods Exp $"

#if defined(apollo) && defined(lint)
# define __attribute(x)		/* XXX ??? */
#endif

#define justfun			/* this is only for fun, but everyone needs some fun! */
#undef  obsolete		/* old code left as a reminder */
#undef  notyet			/* new code for possible future use */

#if defined(__NetBSD__) && defined(__GNUC__)
# define unix	1		/* avoid bogus ``"unix" is no longer predefined'' warning */
#endif

#if (defined(sun) && defined(unix)) || (defined(__sun__) && defined(__unix__))
# define TIME_WITH_SYS_TIME	1
#endif

#if defined(__sun__) && defined(__GNUC__) /* && defined(__sunos4__) */
# define __USE_FIXED_PROTOTYPES__
#endif

#if defined(BSD) || defined(__bsdi__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# ifndef TIME_WITH_SYS_TIME
#  define TIME_WITH_SYS_TIME	1
# endif
# include <sys/cdefs.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>

#if !defined(WINNT)
# include <sys/param.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/in_systm.h>
# include <netinet/ip.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if defined(_AIX)
# include <sys/select.h>	/* needed for fd_set */
#endif

#include <netdb.h>
#undef NOERROR			/* in <sys/streams.h> on solaris 2.x */
#include <arpa/nameser.h>
#include <resolv.h>

#ifdef __STDC__
# include <stdarg.h>
#else
# include <varargs.h>
#endif

#include "port.h"		/* various portability definitions */
#include "conf.h"		/* various configuration definitions */
#include "type.h"		/* types should be in <arpa/nameser.h> */
#include "exit.h"		/* exit codes come from <sysexits.h> */

#ifndef NO_DATA
# define NO_DATA	NO_ADDRESS	/* used here only in case authoritative */
#endif

#define NO_RREC		(NO_DATA + 1)	/* non-authoritative NO_DATA */
#define NO_HOST		(NO_DATA + 2)	/* non-authoritative HOST_NOT_FOUND */
#define QUERY_REFUSED	(NO_DATA + 3)	/* query explicitly refused by server */
#define SERVER_FAILURE	(NO_DATA + 4)	/* instead of TRY_AGAIN upon SERVFAIL */
#define HOST_NOT_CANON	(NO_DATA + 5)	/* host name is not canonical */
#define CACHE_ERROR	(NO_DATA + 6)	/* to signal local cache I/O errors */

#define T_NONE		0		/* yet unspecified resource record type */
#define T_FIRST		T_A		/* first possible type in resource record */
#define T_LAST		(T_IXFR - 1)	/* last  possible type in resource record */

#ifndef NOCHANGE
# define NOCHANGE	0xf		/* compatibility with older BIND versions */
#endif

#define NOT_DOTTED_QUAD	((ipaddr_t) -1)
#define BROADCAST_ADDR	((ipaddr_t) 0xffffffff)
#define LOCALHOST_ADDR	((ipaddr_t) 0x7f000001)

#if !defined(PACKETSZ) && defined(IP_MAXPACKET)
# define PACKETSZ	IP_MAXPACKET
#endif

#if PACKETSZ > 8192
# define MAXPACKET	PACKETSZ	/* PACKETSZ should be the max udp size (512) */
#else
# define MAXPACKET	8192		/* but tcp packets can be considerably larger */
#endif

typedef union {
	HEADER header;			/* from <arpa/nameser.h> */
	u_char packet[MAXPACKET];
} querybuf_t;

#ifndef HFIXEDSZ
# define HFIXEDSZ	12		/* actually sizeof(HEADER) */
#endif

#define MAXDLEN		(MAXPACKET - HFIXEDSZ)	/* upper bound for dlen */

#include "rrec.h"			/* resource record structures */

#define input			/* read-only input parameter */
#define output			/* modified output parameter */

#define MAXINT8		255
#define MAXINT16	65535

#define HASHSIZE	2003	/* size of various hash tables */

#if !defined(errno)		/* XXX who defines this? */
extern int errno;
#endif

#if !defined(h_errno)		/* XXX who defines this? */
extern int h_errno;		/* defined in the resolver library */
#endif

#ifndef _res
extern res_state_t _res;	/* defined in res_init.c */
#endif

#include "defs.h"		/* declaration of functions */

#define plural(n)	(((n) == 1) ? "" : "s")
#define plurale(n)	(((n) == 1) ? "" : "es")

#define is_xdigit(c)	(isascii(c) && isxdigit(c))
#define is_digit(c)	(isascii(c) && isdigit(c))
#define is_print(c)	(isascii(c) && isprint(c))
#define is_space(c)	(isascii(c) && isspace(c))
#define is_alnum(c)	(isascii(c) && isalnum(c))
#define is_upper(c)	(isascii(c) && isupper(c))

#define lowercase(c)	(is_upper(c) ? tolower(c) : (c))
#define hexdigit(c)	(((c) < 10) ? '0' + (c) : 'A' + (c) - 10);

#define bitset(a, b)	(((a) & (b)) != 0)
#define sameword(a, b)	(strcasecmp(a, b) == 0)
#define samepart(a, b)	(strncasecmp(a, b, strlen(b)) == 0)
#define samehead(a, b)	(strncasecmp(a, b, sizeof(b) - 1) == 0)

#define zeroname(a)	(samehead(a, "0.") || samehead(a, "255."))
#define fakename(a)	(samehead(a, "localhost.") || samehead(a, "loopback."))
#define nulladdr(a)	(((a) == 0) || ((a) == BROADCAST_ADDR))
#define fakeaddr(a)	(nulladdr(a) || ((a) == htonl(LOCALHOST_ADDR)))
#define incopy(a)	*((const struct in_addr *) (a))
#define querysize(n)	(((n) > sizeof(querybuf_t)) ? ((int) sizeof(querybuf_t)) : (n))

#define newlist(a, n, t) (t *) xalloc((ptr_t *) (a), (size_t) ((n) * sizeof(t)))
#define newstruct(t)	(t *) xalloc((ptr_t *) NULL, (size_t) (sizeof(t)))
#define newstring(s)	(char *) xalloc((ptr_t *) NULL, (size_t) (strlen(s) + 1))
#define newstr(s)	strcpy(newstring(s), s)
#define xfree(a)	(void) free((ptr_t *) (a))

#define strlength(s)	(int) strlen(s)
#define in_string(s, c)	(strchr(s, c) != NULL)
#define in_label(a, b)	(((a) > (b)) && ((a)[-1] != '.') && ((a)[1] != '.'))
#define is_quoted(a, b)	(((a) > (b)) && ((a)[-1] == '\\'))
#define is_empty(s)	(((s) == NULL) || ((s)[0] == '\0'))
