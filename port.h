/*
** Various portability definitions.
**
*/

#ident "@(#)host:$Name:  $:$Id: port.h,v 1.21 2011-08-17 02:18:51 -0800 woods Exp $"
/*
 * from: @(#)port.h              e07@nikhef.nl (Eric Wassenaar) 991328
 */

#if (defined(__SVR4) || defined(__svr4) || defined(SVR4) || defined(svr4)) && !defined(__svr4__)
# define __svr4__	1
#endif

#if defined(__APPLE__) && defined(__MACH__) && !defined(__darwin__)
# define __darwin__	1
#endif

#if defined(SYSV) || (defined(sun) && defined(unix) && !defined(__svr4__))
# define HAVE_VOID_MALLOC 1
# define SYSV_SETVBUF	1
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
# define HAVE_MEMORY_H	1
#endif

#if defined(WINNT)
# define SYSV_SETVBUF	1
#endif

#if defined(__hpux) || defined(hpux)
# define HAVE_VOID_MALLOC 1	/* XXX necessary?  not on any modern HP/UX */
# define SYSV_SETVBUF	1
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
# define HAVE_MEMORY_H	1
#endif

#if defined(sgi)
# define HAVE_VOID_MALLOC 1	/* XXX necessary?  not on any modern IRIX */
# define HAVE_UNISTD_H	1
#endif

#if defined(linux) || defined(__linux__)
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
#endif

#if defined(bsdi) || defined(__bsdi__)
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__darwin__)
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
#endif

#if defined(NeXT)
# define HAVE_VOID_MALLOC 1	/* XXX necessary?  not likely! */
#endif

#if !defined(sgi) && !defined(__STDC__)
/* damned SGI compiler complains if you undef an ANSI thing!!! */
# ifdef	NULL			/* make sure NULL is 0 */
#  undef NULL
# endif
# define NULL		0	/* the one true C `nil' pointer value */
#endif

/*
** Distinguish between various BIND releases.
*/

/*
 * Every other conceivable version of the BIND-based resolvers should have one
 * or both of __BIND and/or __NAMESER defined to define their API version.
 */
#if !defined(__BIND) && !defined(__NAMESER)
# define BIND_4_8	1	/* XXX this should be ``#include "ERROR!!!"''*/
#endif

#if !defined(__RES)
# define __RES		0	/* XXX this should be ``#include "ERROR!!!"''*/
#endif

/*
** Define constants for fixed sizes.
*/

#ifndef INT16SZ
# define INT16SZ	2	/* for systems without 16-bit ints */
#endif
#ifndef INT32SZ
# define INT32SZ	4	/* for systems without 32-bit ints */
#endif
#ifndef INADDRSZ
# define INADDRSZ	4	/* for sizeof(struct inaddr) != 4 */
#endif

#ifndef	IPNGSIZE
# define IPNGSIZE	16	/* 128 bit ip v6 address size */
#endif

/*
** The following should depend on existing definitions.
*/

typedef int		bool_t;		/* boolean type */

#undef TRUE				/* SunOS-5 defines this in <rpc/types.h> */
#define TRUE		1
#undef FALSE				/* SunOS-5 defines this in <rpc/types.h> */
#define FALSE		0

#if !defined(STDC_HEADERS) && defined(__STDC__)
# define STDC_HEADERS	1
#endif

#ifndef STDIN_FILENO
# define STDIN_FILENO	0
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO	1
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO	2
#endif

/* XXX should this use __RES instead of __NAMESER?  In addition to? */
#if !defined(HAVE_INET_ATON) && \
    ((defined(__BIND) && (__BIND - 0) >= 19950621) || \
     (defined(__NAMESER) && (__NAMESER - 0) >= 19961001) || \
     (defined(BSD4_3) && !defined(BSD4_4)) || \
     (defined(BSD) && (BSD >= 199103)))
# define HAVE_INET_ATON	1
#endif

/*
 * getipnodeby*() and freehostent() were added in BIND-8.2.2 (19991006)
 *
 * Why does everyone have to break defined APIs?
 *
 * FreeBSD added getipnodeby*() separately, obtaining them from KAME, but
 * without adjusting their resolver API version number (leaving it at the
 * BIND-8.1.2 level of 19961001)
 *
 * GNU LibC has a horrible mis-mash of half-baked header files and mangled
 * resolver subroutines, at least as of 2.3.x.  E.g. there's a __NAMESER define
 * in <arpa/nameser.h> indicating it to be BIND-8.2.2 compatible (19991006), but
 * there's no getipnodebyname() in sight.  __RES is also defined as 19991006.
 * Even worse the GLIBC implementation of gethostbyaddr() is totally broken and
 * does not return multiple PTRs.
 *
 * NetBSD 3.x and newer incorporates the BIND-8 resolver (from the BIND-9
 * distribution), but sadly does not include the getipnodeby*() API which is
 * still provided in libbind.  If you're going to do something, do it right!
 *
 * PLEASE always build and link against BIND-8.4.0 or newer!
 */
#if defined(__NAMESER) && (!defined(__GLIBC__) || ((__RES - 0) > 19991006)) && \
	(((__NAMESER - 0) >= 19991006 && !defined(__NetBSD__)) || \
	 (defined(__FreeBSD__) && (__NAMESER - 0) >= 19961001))
# define HAVE_GETIPNODEBYNAME	1
# define HAVE_GETIPNODEBYADDR	1
# define HAVE_FREEHOSTENT	1
#endif

/*
 * A special version of res_send() is included in send.c.  It is highly
 * desirable to use it when compiling with ancient resolvers, but much less
 * desirable for modern resolvers unless your vendor has corrupted your DNS
 * resolver with non-DNS lookup methods (especially if the target system 'host'
 * will be run on uses such foreign schemes -- host is intended to be used only
 * with the DNS)
 *
 * NOTE:  -DHOST_RES_SEND will not usually work with newer BIND-8 libbind.
 */
#if !defined(HOST_RES_SEND) && \
    (!defined(__BIND) || (__BIND - 0) < 19950621) && \
    (!defined(__NAMESER) || (__NAMESER - 0) < 19961001)
# define HOST_RES_SEND	1	/* use the special host res_send() */
#endif

/*
 * For now we assume these are constant in all resolvers...
 *
 * Note these are arbitrary values used to size the data structures filled by
 * the standard get*by*() APIs.  We use these defined values to print error
 * messages should an RR set be encountered which might not be fully handled by
 * those APIs.
 */
#define MAXALIAS	35	/* maximum aliases count from gethnamaddr.c */
#define MAXADDRS	35	/* maximum address count from gethnamaddr.c */

/*
 * Prefix for messages on stdout in debug mode.
 */
#if !defined(BIND_4_8)
# define DEBUG_PREFIX	";; "
#else
# define DEBUG_PREFIX	""
#endif

#if defined(BIND_4_8) || defined(OLD_RES_STATE)
typedef struct state		res_state_t;
#else
typedef struct __res_state	res_state_t;
#endif

#if defined(BIND_4_8)
typedef struct rrec	rrec_t;
#else
# if (defined(__BIND) && (__BIND - 0) >= 19950621) || \
     (defined(__NAMESER) && (__NAMESER - 0) >= 19961001)
typedef u_char		rrec_t;
# else
typedef char		rrec_t;
# endif
#endif

#if (defined(__BIND) && (__BIND - 0) >= 19950621) || \
    (defined(__NAMESER) && (__NAMESER - 0) >= 19961001)
typedef u_char		qbuf_t;
#else
typedef char		qbuf_t;
#endif

#if (defined(__BIND) && (__BIND - 0) >= 19950621) || \
    (defined(__NAMESER) && (__NAMESER - 0) >= 19961001)
typedef char		nbuf_t;
#else
typedef u_char		nbuf_t;
#endif

/*
 * I'm not sure when GNU LibC first got ns_*t*(), but for certain 2.1 with
 * __BIND at 19960801 doesn't have them.
 */
#if !defined(__NAMESER) && \
    (!defined(__GLIBC__) || \
     (defined(__GLIBC__) && defined(__BIND) && (__BIND - 0) <= 19960801))
# define ns_get16(src)		_getshort(src)
# define ns_get32(src)		_getlong(src)
# define ns_put16(src, dst)	__putshort(src, dst)
# define ns_put32(src, dst)	__putlong(src, dst)
#endif

/*
 * XXX we should probably convert to using the more standard "in_addr_t"
 */
#if !defined(__in_addr_t)
# if defined(_IN_ADDR_T) || defined(in_addr_t)
typedef in_addr_t	ipaddr_t;
# else
/* note:  assume the preprocessor is using at least 32-bit unsigned long (ISO C 1990) */
/* xxx if we knew we had uint32_t... */
#  if UINT_MAX == 0xFFFFFFFFU
typedef unsigned int	ipaddr_t;
#  elif ULONG_MAX == 0xFFFFFFFFU
typedef unsigned long	ipaddr_t;
#  else
#   include "houston, we have a problem!"
#  endif
# endif
#else
typedef __in_addr_t	ipaddr_t;
#endif

/*
 * FreeBSD (and Darwin in its image) is a bit brain-dead in the way they do
 * their multiple typedef avoidance -- i.e. they still follow the ancient
 * 4.4BSD style of using the fact that _BSD_SOCKLEN_T_ is NOT defined in order
 * to typedef socklen_t at the earliest point it's needed.  However they leave
 * no means for applications to know if the typedef has already been done.
 *
 * The most elegant way to protect typedefs is to prefix the type name with
 * "__" for the typedef and then use a CPP #define to map the true unprefixed
 * name to the actual typedef name.  This way the presence of the type name as
 * a #define tells us that the typedef for it has already been done.
 *
 * All the other schemes are just inelegant hacks, but at least they're better
 * than having to know the details of individual OS library implementations!
 *
 * FYI: In NetBSD socklen_t came into use just before 1.3J:
 *
 *	(__NetBSD_Version__ - 0) > 103100000
 *
 * Not sure when GNU LibC added socklen_t, but it's in 2.1 at least.
 */
#if (defined(__FreeBSD__) || defined(__darwin__)) && defined(_BSD_SOCKLEN_T_)
# include "ERROR: something's wrong with the #includes above!"
#endif
/* Sigh, standards are such wonderful things.... */
#if !defined(socklen_t) && \
    !defined(__FreeBSD__) && !defined(__darwin__) && \
    !defined(_SOCKLEN_T) && !defined(__socklen_t_defined) && \
    (!defined(__GLIBC__) || (__GLIBC__ - 0) < 2) && \
    (!defined(__GLIBC_MINOR__) || (__GLIBC_MINOR__ - 0) < 1)
# if (/* SunOS-4 gcc */defined(__sun__) && !defined(__svr4__)) || \
     (/* SunOS-4 cc */defined(sun) && defined(unix) && !defined(__svr4__)) || \
     (/* 4.3BSD */defined(BSD) && ((BSD - 0) > 0) && ((BSD - 0) < 199506))
typedef int		__socklen_t;	/* 4.3BSD and older */
# else
typedef size_t		__socklen_t;	/* P1003.1g socket-related datum length */
# endif
typedef __socklen_t	socklen_t;
# define socklen_t	__socklen_t
#endif

/*
 * BSD Socket API buffer length type.
 *
 * Deal with the other parts of the P1003.1g API change which the POSIX
 * committee didn't seem to address.  I.e. use this for the "buflen" or "len"
 * parameters of functions such as send(2), sendto(2), recv(2), recvfrom(2),
 * etc.  (not the address length, which should be a socklen_t, just the buffer
 * length).
 *
 * Unfortunately this doesn't deal with the problem that the passed in length
 * is now a size_t width integer, but the returned type is only a ssize_t width
 * integer....  Standards.  Sigh.
 *
 * Perhaps the "defined(__sun__)" shouldn't be there on the line with
 * !defined(_SOCKLEN_T) -- I need a pure SysVr4 system to check on.
 */
#if !defined(sock_buflen_t)			/* silly dreamer! */
# if (/* SunOS-4 gcc */defined(__sun__) && !defined(__svr4__)) || \
     (/* SunOS-4 cc */defined(sun) && defined(unix) && !defined(__svr4__)) || \
     (/* SunOS-5.9 */defined(__sun__) && defined(__svr4__) && !defined(_SOCKLEN_T)) || \
     (/* 4.3BSD */defined(BSD) && ((BSD - 0) > 0) && ((BSD - 0) < 199506))
typedef int		__sock_buflen_t;	/* 4.3BSD and older used int */
# else
typedef size_t		__sock_buflen_t;	/* P1003.1g adherents use size_t */
# endif
typedef __sock_buflen_t	sock_buflen_t;
# define sock_buflen_t	__sock_buflen_t
#endif

#if defined(apollo) || defined(_BSD_SIGNALS)
typedef int	sigtype_t;
#else
typedef void	sigtype_t;
#endif

#if defined(HAVE_VOID_MALLOC) || defined(__STDC__)
typedef void	ptr_t;		/* generic pointer type */
typedef void	free_t;
#else
/*
 * These may not always be 100% correct for old non-STDC systems....
 */
typedef char	ptr_t;		/* generic pointer type */
typedef int	free_t;
#endif

#if defined(SYSV_SETVBUF) || defined(__STDC__)
# define linebufmode(a)	(void) setvbuf(a, (char *) NULL, _IOLBF, (size_t) 0)
#else
# define linebufmode(a)	(void) setlinebuf(a)
#endif

#ifdef ULTRIX_RESOLV
# define nslist(i)	_res.ns_list[i].addr
#else
# define nslist(i)	_res.nsaddr_list[i]
#endif

#ifdef fp_nquery
# define pr_query(a, n, f)	fp_nquery(a, n, f)
#else
# define pr_query(a, n, f)	fp_query(a, f)
#endif

#if defined(sun) && defined(NO_YP_LOOKUP)
# define gethostbyname	(struct hostent *) res_gethostbyname
# define gethostbyaddr	(struct hostent *) res_gethostbyaddr
#endif

#if defined(__svr4__)
# define jmp_buf	sigjmp_buf
# define setjmp(e)	sigsetjmp(e, 1)
# define longjmp(e, n)	siglongjmp(e, n)
#endif

/*
** Very specific definitions for certain platforms.
*/

#if defined(WINNT)
# define NO_CONNECTED_DGRAM
#endif

#if defined(WINNT)
# undef  linebufmode
# define linebufmode(a)	(void) setvbuf(a, (char *) NULL, _IONBF, 0)
#endif

#if defined(WINNT)
# ifndef strcasecmp
#  define strcasecmp	_stricmp
# endif
# ifndef strncasecmp
#  define strncasecmp	_strnicmp
# endif
#endif /*WINNT*/

#if defined(WINNT)
# define setalarm(n)
# define setsignal(s, f)
#else
# define setalarm(n)		(void) alarm((unsigned int) (n))
# define setsignal(s, f)	(void) signal(s, f)
#endif

#if defined(WINNT)
# ifndef errno
#  define errno		WSAGetLastError()
# endif
# ifndef h_errno
#  define h_errno	WSAGetLastError()
# endif
#endif /*WINNT*/

#if defined(WINNT)
# define set_errno(n)	WSASetLastError(n)
# define set_h_errno(n)	WSASetLastError(n)
#else
# define set_errno(n)	errno = (n)
# define set_h_errno(n)	h_errno = (n)
#endif

#if defined(WINNT)
# undef  EINTR
# define EINTR		WSAEINTR
# undef  EWOULDBLOCK
# define EWOULDBLOCK	WSAEWOULDBLOCK
# undef  ETIMEDOUT
# define ETIMEDOUT	WSAETIMEDOUT
# undef  ECONNRESET
# define ECONNRESET	WSAECONNRESET
# undef  ECONNREFUSED
# define ECONNREFUSED	WSAECONNREFUSED
# undef  ENETDOWN
# define ENETDOWN	WSAENETDOWN
# undef  ENETUNREACH
# define ENETUNREACH	WSAENETUNREACH
# undef  EHOSTDOWN
# define EHOSTDOWN	WSAEHOSTDOWN
# undef  EHOSTUNREACH
# define EHOSTUNREACH	WSAEHOSTUNREACH
# undef  EADDRINUSE
# define EADDRINUSE	WSAEADDRINUSE
#endif /*WINNT*/

#if defined(WINNT)
HANDLE hReadWriteEvent;
#endif

#if defined(WINNT) && !defined(__STDC__)
# define __STDC__
#endif

#ifndef __P		/* in *BSD's <sys/cdefs.h>, included by everything! */
# if ((__STDC__ - 0) > 0) || defined(__cplusplus)
#  define __P(protos)	protos		/* full-blown ANSI C */
# else
#  define __P(protos)	()		/* traditional C */
# endif
#endif

#ifndef const		/* in *BSD's <sys/cdefs.h>, included by everything! */
# if ((__STDC__ - 0) <= 0) || defined(apollo)
#  define const		/* NOTHING */
# endif
#endif

#ifndef volatile	/* in *BSD's <sys/cdefs.h>, included by everything! */
# if !defined(__STDC__) && !defined(__cplusplus)/* ((__STDC__ - 0) > 0) ??? */
#  define volatile	/* NOTHING most compilers won't optimize global variables */
# endif
#endif

#ifdef __STDC__
# define VA_START(args, lastarg)       va_start(args, lastarg)
#else
# define VA_START(args, lastarg)       va_start(args)
#endif

/*
 * Macro to test if we're using a GNU C compiler of a specific vintage
 * or later, for e.g. features that appeared in a particular version
 * of GNU C.  Usage:
 *
 *	#if __GNUC_PREREQ__(major, minor)
 *	...cool feature...
 *	#else
 *	...delete feature...
 *	#endif
 */
#if defined(__GNUC__) && !defined(__GNUC_PREREQ__)
#define	__GNUC_PREREQ__(x, y)						\
	((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) ||			\
	 (__GNUC__ > (x)))
#elif !defined(__GNUC_PREREQ__)
#define	__GNUC_PREREQ__(x, y)	0
#endif

/* adapt for GCC's lack of adherence to lint's standard ARGSUSED comment */
#if !__GNUC_PREREQ__(2, 5) && !defined(__attribute__)
# define __attribute__(x)	/* delete __attribute__ if non-gcc or old gcc */
#endif
