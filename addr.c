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

#ident "@(#)host:$Name:  $:$Id: addr.c,v 1.12 2007-01-29 02:30:46 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)addr.c	e07@nikhef.nl (Eric Wassenaar) 990605";
#endif

#include "host.h"
#include "glob.h"

static bool_t check_name_addr	__P((char *, ipaddr_t));
static bool_t check_addr_name	__P((struct in_addr, char *));


/*
** CHECK_ADDR -- Check whether reverse address mappings revert to host
** -------------------------------------------------------------------
**
**	Returns:
**		TRUE if all addresses of host map back to host.
**		FALSE otherwise.
*/

bool_t
check_addr(name)
	input char *name;		/* host name to check addresses for */
{
	struct hostent *hp;
	struct in_addr *inaddr = NULL;
	register unsigned int i;
	unsigned int naddrs = 0;
	unsigned int matched = 0;
	char *hname;
	char hnamebuf[MAXDNAME + 1];
	int my_h_errno;

	if (verbose)
		printf("Checking reverse mapping consistency for host %s\n", name);

	/*
	 * First just verify whether the mapped host name is canonical or not.
	 */
	print_level++;
	if ((my_h_errno = canonical(name)) != 0 && !debug) {
		set_h_errno(my_h_errno);
		ns_error(name, T_A, C_IN, server);
		/* no real point in giving up here yet though.... */
	}
	print_level--;

	/*
	 * Look up the specified host to fetch all of its addresses.
	 */
	/*
	 * XXX ARGH!  We should avoid using the upper-level BIND resolver for
	 * this query so that we can ensure we always get _all_ of the answers!
	 *
	 * XXX also need to deal properly with IPv6 too....
	 */
#if defined(HAVE_GETIPNODEBYNAME)
	if (!(hp = getipnodebyname(name, AF_INET, AI_ALL | AI_V4MAPPED, &my_h_errno))) {
		set_h_errno(my_h_errno);
		ns_error(name, T_A, C_IN, server);
		return (FALSE);
	}
#else
	if (!(hp = gethostbyname(name))) {
		ns_error(name, T_A, C_IN, server);
		return (FALSE);
	}
#endif

	hname = strncpy(hnamebuf, hp->h_name, (size_t) MAXDNAME);
	hname[MAXDNAME] = '\0';

	if (!sameword(hname, name))
		pr_warning("primary hostname with the same address as %s is %s", name, hname);

	/* XXX are there any systems where h_addr is not just h_addr_list[0]? */

	for (i = 0; hp->h_addr_list[i]; i++) {
		naddrs++;
		if (verbose) {
			printf("Hostname %s maps to address %s\n",
			       hnamebuf, inet_ntoa(incopy(hp->h_addr_list[i])));
		}
	}

	if (verbose && naddrs) {
		printf("Found %d address%s for host %s\n",
		       naddrs, plurale(naddrs), hname);
	}
	if (!(inaddr = malloc(naddrs * sizeof(*inaddr)))) {
		sys_error("malloc(%d): failed: ", naddrs * sizeof(*inaddr), strerror(errno));
		return (FALSE);
	}
	for (i = 0; hp->h_addr_list[i]; i++)
		inaddr[i] = incopy(hp->h_addr_list[i]);

	/*
	 * XXX this check only detects something if the libbind host is linked
	 * with has been patched to either dynamically allocate its internal
	 * arrays, or at least has had them expanded beyond the norm.
	 */
	if (naddrs > MAXADDRS) {
		pr_error("%s: most resolvers only support %d A records per RR set, found %d.",
			 hname, MAXADDRS, naddrs);
	}

	/*
	 * Map back the addresses found, and check whether they revert to host.
	 */
	for (i = 0; i < naddrs; i++) {
		if (check_addr_name(inaddr[i], hname))
			matched++;
	}

	free((ptr_t *) inaddr);

#if defined(HAVE_FREEHOSTENT)
	freehostent(hp);
#endif

	if (matched != naddrs)
		pr_error("%s addresses for hostname %s have a matching hostname.",
			 matched ? "Not all of the" : "None of the",
			 hname);

	return ((matched == naddrs) ? TRUE : FALSE);
}

/*
** CHECK_ADDR_NAME -- Check whether reverse address mappings for an address belong to host
** ---------------------------------------------------------------------------------------
**
**	Returns:
**		TRUE if the given address of host maps back to host.
**		FALSE otherwise.
*/

static bool_t
check_addr_name(inaddr, name)
	input struct in_addr inaddr;	/* address of host to map back */
	input char *name;		/* name of host to check */
{
	struct hostent *hp;
	register unsigned int i;
	char *iname, inamebuf[MAXDNAME + 1];
	unsigned int matched = 0;
#if defined(HAVE_GETIPNODEBYADDR)
	int my_h_errno;
#endif

	iname = strcpy(inamebuf, inet_ntoa(inaddr));

	if (verbose)
		printf("Checking if address %s has the hostname %s\n", iname, name);

	/*
	 * Fetch the reverse mapping of the given host address.
	 */
	/*
	 * XXX ARGH!  We should avoid using the upper-level BIND resolver for
	 * this query so that we can ensure we always get _all_ of the answers!
	 *
	 * XXX also need to deal properly with IPv6 too....
	 */
#if defined(HAVE_GETIPNODEBYADDR)
	if (!(hp = getipnodebyaddr((void *) &inaddr, sizeof(inaddr), AF_INET, &my_h_errno))) {
		set_h_errno(my_h_errno);
		ns_error(name, T_PTR, C_IN, server);
		return (FALSE);
	}
#else
	if (!(hp = gethostbyaddr((char *) &inaddr, INADDRSZ, AF_INET))) {
		ns_error(iname, T_PTR, C_IN, server);
		return (FALSE);
	}
#endif

	/*
	 * Check whether the ``official'' hostname matches.  This is simply the
	 * name in the first (or only) PTR record encountered.
	 */
	if (sameword(hp->h_name, name))
		matched++;

	if (verbose) {
		printf("%s address %s maps to hostname %s\n",
		       name, iname, hp->h_name);
	}

	/*
	 * A match may also be found among the ``aliases''.
	 *
	 * They are available (as of BIND 4.9) when multiple PTR records are
	 * used.
	 *
	 * XXX loop is not strictly necessary if (!verbose && matched)
	 */
	for (i = 0; hp->h_aliases[i]; i++) {
		if (sameword(hp->h_aliases[i], name))
			matched++;

		if (verbose) {
			printf("%s address %s maps to hostname %s\n",
			       name, iname, hp->h_aliases[i]);
		}
	}

	if (!matched) {
		pr_error("Hostname %s does not belong to address %s",
			 name, iname);
	} else if (matched > 1) {	/* XXX this should be impossible */
		pr_warning("Hostname %s does not belong to address %s",
			   name, iname);
	}

#if defined(HAVE_FREEHOSTENT)
	freehostent(hp);
#endif

	return (matched ? TRUE : FALSE);
}

/*
** CHECK_NAME -- Check whether address belongs to host addresses
** -------------------------------------------------------------
**
**	Returns:
**		TRUE if given address was found among host addresses.
**		FALSE otherwise.
*/

bool_t
check_name(addr)
	input ipaddr_t addr;		/* address of host to check */
{
	struct hostent *hp;
	register unsigned int i;
	struct in_addr inaddr;
	char *iname, inamebuf[MAXDNAME + 1];
	char *hname, hnamebuf[MAXDNAME + 1];
	char *aname, *anamebuf = NULL;
	unsigned int naliases = 0;
	unsigned int matched = 0;
#if defined(HAVE_GETIPNODEBYADDR)
	int my_h_errno;
#endif

	inaddr.s_addr = addr;
	iname = strcpy(inamebuf, inet_ntoa(inaddr));

	if (verbose)
		printf("Checking forward mapping consistency for address %s\n", iname);

	/*
	 * Check whether the address is registered by fetching its host name.
	 */
	/*
	 * XXX ARGH!  We should avoid using the upper-level BIND resolver for
	 * this query so that we can ensure we always get _all_ of the answers!
	 *
	 * XXX also need to deal properly with IPv6 too....
	 */
#if defined(HAVE_GETIPNODEBYADDR)
	if (!(hp = getipnodebyaddr((void *) &inaddr, sizeof(inaddr), AF_INET, &my_h_errno))) {
		set_h_errno(my_h_errno);
		ns_error(iname, T_PTR, C_IN, server);
		return (FALSE);
	}
#else
	if (!(hp = gethostbyaddr((char *) &inaddr, INADDRSZ, AF_INET))) {
		ns_error(iname, T_PTR, C_IN, server);
		return (FALSE);
	}
#endif

	hname = strncpy(hnamebuf, hp->h_name, (size_t) MAXDNAME);
	hname[MAXDNAME] = '\0';

	if (verbose)
		printf("Address %s maps to hostname %s\n", iname, hname);

	/*
	 * In case of multiple PTR records, additional names are stored in
	 * h_aliases.
	 */
	for (i = 0; hp->h_aliases[i]; i++) {
		naliases++;
		if (verbose)
			printf("Address %s maps to hostname %s\n", iname, hp->h_aliases[i]);
	}

	if (verbose && naliases) {
		printf("Found %d hostname%s for %s\n",
		       naliases + 1, plural(naliases + 1), iname);
	}
	if (naliases) {
		/* XXX this is big and sparse and wasteful, but what the heck... */
		if (!(anamebuf = calloc((size_t) naliases, (size_t) (MAXDNAME + 1)))) {
			sys_error("calloc(%u, %d): failed: ", naliases, (MAXDNAME + 1), strerror(errno));
			return (FALSE);
		}
		for (i = 0; hp->h_aliases[i]; i++) {
			aname = strncpy(&anamebuf[i * (MAXDNAME + 1)], hp->h_aliases[i], (size_t) MAXDNAME);
			aname[MAXDNAME] = '\0';
		}
	}
	/*
	 * XXX this check only detects something if the libbind host is linked
	 * with has been patched to either dynamically allocate its internal
	 * arrays, or at least has had them expanded beyond the norm.
	 */
	if (naliases > MAXALIAS) {
		/* report the full number of PTRs so the message makes sense */
		pr_error("%s: most resolvers only support %d PTR records per RR set, found %d.",
			 iname, MAXALIAS, naliases + 1);
	}

	/*
	 * Check whether the given address belongs to each of the host names
	 */
	if (check_name_addr(hname, addr))
		matched++;

	for (i = 0; i < naliases; i++) {
		aname = &anamebuf[i * (MAXDNAME + 1)];
		if (check_name_addr(aname, addr))
			matched++;
	}

	free((ptr_t *) anamebuf);

#if defined(HAVE_FREEHOSTENT)
	freehostent(hp);
#endif

	if (matched != (naliases + 1))
		pr_error("%s hostnames for address %s have a matching address.",
			 matched ? "Not all of the" : "None of the",
			 iname);

	return ((matched == (naliases + 1)) ? TRUE : FALSE);
}

/*
** CHECK_NAME_ADDR -- Check whether address belongs to host
** --------------------------------------------------------
**
**	Returns:
**		TRUE if given address was found among host addresses.
**		FALSE otherwise.
**
** XXX We could probably replace this function with a with a variant of
** canonical() that had a slightly different API -- one that would optionally
** also check that "addr" was amongst the T_A records listed for the host.
*/

static bool_t
check_name_addr(name, addr)
	input char *name;		/* name of host to check */
	input ipaddr_t addr;		/* address should belong to host */
{
	struct hostent *hp;
	register unsigned int i;
	struct in_addr inaddr;
	char *iname, inamebuf[MAXDNAME+1];
	unsigned int matched = 0;
	int my_h_errno;

	inaddr.s_addr = addr;
	iname = strcpy(inamebuf, inet_ntoa(inaddr));

	if (verbose) {
		printf("Checking if %s is a hostname with an address matching %s\n",
		       name, iname);
	}

	/*
	 * First just verify whether the mapped host name is canonical or not.
	 */
	print_level++;
	if ((my_h_errno = canonical(name)) != 0 && !debug) {
		set_h_errno(my_h_errno);
		ns_error(name, T_A, C_IN, server);
		/* no real point in giving up here yet though.... */
	}
	print_level--;

	/*
	 * Look up the host 'name' to fetch its addresses.
	 */
	/*
	 * XXX ARGH!  We should avoid using the upper-level BIND resolver for
	 * this query so that we can ensure we always get _all_ of the answers!
	 *
	 * XXX also need to deal properly with IPv6 too....
	 */
#if defined(HAVE_GETIPNODEBYNAME)
	if (!(hp = getipnodebyname(name, AF_INET, AI_ALL | AI_V4MAPPED, &my_h_errno))) {
		set_h_errno(my_h_errno);
		ns_error(name, T_A, C_IN, server);
		return (FALSE);
	}
#else
	if (!(hp = gethostbyname(name))) {
		ns_error(name, T_A, C_IN, server);
		return (FALSE);
	}
#endif

	/*
	 * Check whether the given address is listed among the known addresses.
	 */
	for (i = 0; hp->h_addr_list[i]; i++) {
		inaddr = incopy(hp->h_addr_list[i]);

		if (inaddr.s_addr == addr)
			matched++;

		if (verbose) {
			printf("Hostname %s maps to address %s\n",
			       name, inet_ntoa(inaddr));
		}
	}

	if (!matched) {
		pr_error("address %s does not belong to hostname %s",
			 iname, name);
	} else if (verbose) {
		printf("Address %s is a target of the hostname %s\n",
		       iname, name);
	}
#if defined(HAVE_FREEHOSTENT)
	freehostent(hp);
#endif

	return (matched ? TRUE : FALSE);
}
