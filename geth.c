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

#ident "@(#)host:$Name:  $:$Id: geth.c,v 1.5 2003-03-29 19:49:52 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)geth.c	e07@nikhef.nl (Eric Wassenaar) 990605";
#endif

#include "host.h"
#include "glob.h"

/*
** GETH_BYNAME -- Wrapper for gethostbyname
** ----------------------------------------
**
**	Returns:
**		Pointer to struct hostent if lookup was successful.
**		NULL otherwise.
**
**	Note. This routine works for fully qualified names only.
**	The entire special res_search() processing can be skipped.
*/

struct hostent *
geth_byname(name)
	input const char *name;		/* name to do forward lookup for */
{
	querybuf_t answer;
	struct hostent *hp;
	register int n;

	if ((hp = gethostbyname(name)))
		return (hp);

	if (verbose > print_level)
		printf("Finding addresses for %s ...\n", name);

	if ((n = get_info(&answer, name, T_A, C_IN)) < 0)
		return (NULL);

	if ((verbose > print_level + 1) && (print_level < 1))
		(void) print_info(&answer, n, name, T_A, C_IN, FALSE);

	return gethostbyname(name);
}

/*
** GETH_BYADDR -- Wrapper for gethostbyaddr
** ----------------------------------------
**
**	Returns:
**		Pointer to struct hostent if lookup was successful.
**		NULL otherwise.
*/

struct hostent *
geth_byaddr(addr, size, family)
	input const char *addr;		/* address to do reverse mapping for */
	input size_t size;		/* size of the address */
	input int family;		/* address family */
{
	char addrbuf[(4 * 4) + sizeof(ARPA_ROOT) + 1];
	char *name = addrbuf;
	const u_char *a = (const u_char *) addr;
	querybuf_t answer;
	struct hostent *hp;
	register int n;

	if (size != INADDRSZ || family != AF_INET)
		return gethostbyaddr(addr, size, family);

	if ((hp = gethostbyaddr(addr, size, family)))
		return (hp);

	/* construct absolute reverse name *without* trailing dot */
	(void) sprintf(addrbuf, "%u.%u.%u.%u.%s",
		       a[3] & 0xff, a[2] & 0xff, a[1] & 0xff, a[0] & 0xff, ARPA_ROOT);

	if (verbose > print_level) {
		printf("Finding reverse mapping for %s ...\n",
		       inet_ntoa(incopy(addr)));
	}

	if ((n = get_info(&answer, name, T_PTR, C_IN)) < 0)
		return (NULL);

	if ((verbose > print_level + 1) && (print_level < 1))
		(void) print_info(&answer, n, name, T_PTR, C_IN, FALSE);

	return gethostbyaddr(addr, size, family);
}
