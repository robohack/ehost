/*
** Various portability definitions.
**
**	@(#)port.h              e07@nikhef.nl (Eric Wassenaar) 991328
*/

#ident "@(#)host:$Name:  $:$Id: port.h,v 1.7 2003-03-30 17:38:57 -0800 woods Exp $"

#if defined(__SVR4) || defined(__svr4__)
# define SVR4
#endif

#if defined(SYSV) || defined(SVR4) || (defined(sun) && defined(unix) && !defined(SVR4))
# define SYSV_MALLOC	1
# define SYSV_MEMSET	1
# define SYSV_SETVBUF	1
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
# define HAVE_MEMORY_H	1
#endif

#if defined(WINNT)
# define SYSV_MALLOC	1
# define SYSV_SETVBUF	1
#endif

#if defined(__hpux) || defined(hpux)
# define SYSV_MALLOC	1
# define SYSV_SETVBUF	1
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
# define HAVE_MEMORY_H	1
#endif

#if defined(sgi)
# define SYSV_MALLOC	1
# define HAVE_UNISTD_H	1
#endif

#if defined(linux) || defined(__linux__)
# define SYSV_MALLOC	1
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
#endif

#if defined(bsdi) || defined(__bsdi__)
# define SYSV_MALLOC	1
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# define SYSV_MEMSET	1
# define HAVE_UNISTD_H	1
# define HAVE_STRING_H	1
#endif

#if defined(NeXT)
# define SYSV_MALLOC	1
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

#if defined(RES_PRF_STATS)
# define BIND_4_9
#else
# define BIND_4_8
#endif

#if defined(__BIND)		/* ((__BIND - 0) > 19950621) */
# define BIND_4_9_3
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

#ifndef STDIN_FILENO
# define STDIN_FILENO	0
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO	1
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO	2
#endif

#if !defined(HAVE_INET_ATON) && \
    (((__BIND - 0) > 19950621) || \
     ((__NAMESER - 0) > 19961001) || \
     defined(BSD4_3) || \
     (defined(BSD) && (BSD >= 199103)))
# define HAVE_INET_ATON
#endif

#if defined(BIND_4_8) || defined(OLD_RES_STATE)
typedef struct state		res_state_t;
#else
typedef struct __res_state	res_state_t;
#endif

#if defined(BIND_4_8)
typedef struct rrec	rrec_t;
#else
# if ((__BIND - 0) > 19950621) || ((__NAMESER - 0) > 19961001)
typedef u_char		rrec_t;
# else
typedef char		rrec_t;
# endif
#endif

#if ((__BIND - 0) > 19950621) || ((__NAMESER - 0) > 19961001)
typedef u_char	qbuf_t;
#else
typedef char	qbuf_t;
#endif

#if ((__BIND - 0) > 19950621) || ((__NAMESER - 0) > 19961001)
typedef char	nbuf_t;
#else
typedef u_char	nbuf_t;
#endif

#ifndef _IPADDR_T
# if defined(__alpha) || defined(BIND_4_9)
typedef u_int	ipaddr_t;
# else
typedef u_long	ipaddr_t;
# endif
#endif

#if defined(apollo) || defined(_BSD_SIGNALS)
typedef int	sigtype_t;
#else
typedef void	sigtype_t;
#endif

#if defined(SYSV_MALLOC) || defined(__STDC__)
typedef void	ptr_t;		/* generic pointer type */
typedef void	free_t;
#else
typedef char	ptr_t;		/* generic pointer type */
typedef int	free_t;
#endif

#ifdef SYSV_MEMSET
# define bzero(a, n)	(void) memset(a, '\0', n)
# define bcopy(a, b, n)	(void) memcpy(b, a, n)
#endif

#ifdef SYSV_SETVBUF
# define linebufmode(a)	(void) setvbuf(a, (char *) NULL, _IOLBF, BUFSIZ)
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

#if defined(SVR4)
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
# define seterrno(n)	WSASetLastError(n)
# define seth_errno(n)	WSASetLastError(n)
#else
# define seterrno(n)	errno = (n)
# define seth_errno(n)	h_errno = (n)
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
