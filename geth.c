/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ident "@(#)host:$Name:  $:$Id: geth.c,v 1.12 2004-10-17 18:39:50 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)geth.c	e07@nikhef.nl (Eric Wassenaar) 990605";
#endif

#include "host.h"
#include "glob.h"

/*
** GETH_BYNAME -- Wrapper for gethostbyname
** ----------------------------------------
**
**	If gethostbyname() fails then try calling get_info().
**
**	Returns:
**		Pointer to struct hostent if lookup was successful.
**		NULL otherwise.
**
**	Note. This routine works for fully qualified names only.
**	The entire special res_search() processing can be skipped.
**
**	XXX needs a "family" parameter!
*/

struct hostent *
geth_byname(name)
	input const char *name;		/* name to do forward lookup for */
{
	struct hostent *hp;
#if defined(HAVE_GETIPNODEBYNAME)
	int my_h_errno;
#endif

	if (verbose > print_level)
		printf("Finding addresses for %s ...\n", name);

	if (debug || verbose > print_level+1) {
		querybuf_t answer;
		register int n;

		if ((n = get_info(&answer, name, T_A, C_IN)) > 0)
			(void) print_info(&answer, n, name, T_A, C_IN, FALSE);
	}
#if defined(HAVE_GETIPNODEBYNAME)
	if (!(hp = getipnodebyname(name, AF_INET, AI_ALL | AI_V4MAPPED, &my_h_errno)))
		set_h_errno(my_h_errno);
#else
	hp = gethostbyname(name);
#endif
	if (!hp && verbose)
		fprintf(stderr, "%s: gethostbyname(%s): %s\n", argv0, name, hstrerror(h_errno));

	return (hp);
}

/*
** GETH_BYADDR -- Wrapper for gethostbyaddr
** ----------------------------------------
**
**	For family == AF_INET if gethostbyaddr() fails then try calling
**	get_info() for the in-addr.arpa name derived from addr.
**
**	Returns:
**		Pointer to struct hostent if lookup was successful.
**		NULL otherwise.
*/

struct hostent *
geth_byaddr(addr, size, family)
	input const char *addr;		/* address to do reverse mapping for */
	input socklen_t size;		/* size of the address */
	input int family;		/* address family */
{
	struct hostent *hp;
#if defined(HAVE_GETIPNODEBYADDR)
	int my_h_errno;
#endif

	if (debug || verbose > print_level) {
		printf("Finding reverse mapping for %s ...\n",
		       inet_ntoa(incopy(addr)));
	}
	if (debug || verbose > print_level+1) {
		char addrbuf[(4 * 4) + sizeof(ARPA_ROOT) + 1];
		char *name;
		const u_char *a = (const u_char *) addr;
		querybuf_t answer;
		register int n;

		/* construct absolute reverse name *without* trailing dot */
		(void) sprintf(addrbuf, "%u.%u.%u.%u.%s",
			       a[3] & 0xff, a[2] & 0xff, a[1] & 0xff, a[0] & 0xff, ARPA_ROOT);
		name = addrbuf;
		if ((n = get_info(&answer, name, T_PTR, C_IN)) > 0)
			(void) print_info(&answer, n, name, T_PTR, C_IN, FALSE);
	}

#if defined(HAVE_GETIPNODEBYADDR)
	if (!(hp = getipnodebyaddr((const void *) addr, size, family, &my_h_errno)))
		set_h_errno(my_h_errno);
#else
	hp = gethostbyaddr(addr, size, family); /* XXX size _SHOULD_ be socklen_t, but may not be... */
#endif
	if (!hp && verbose)
		fprintf(stderr, "%s: gethostbyaddr(%s): %s\n", argv0, inet_ntoa(incopy(addr)), hstrerror(h_errno));

	return (hp);
}

/*
** GETH_FREEHOSTENT -- Wrapper for freehostent
** -------------------------------------------
*/

/* ARGSUSED */
void
geth_freehostent(hp)
	struct hostent *hp
#if !defined(HAVE_FREEHOSTENT)
		GCC_UNUSED_HACK
#endif
	;
{
#if defined(HAVE_FREEHOSTENT)
	freehostent(hp);
#endif
}
