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

#ident "@(#)host:$Name:  $:$Id: list.c,v 1.29 2011-08-17 01:01:00 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)list.c	e07@nikhef.nl (Eric Wassenaar) 991529";
#endif

#include "host.h"
#include "glob.h"

static bool_t check_cache	__P((char *, char *));


/*
 * Nameserver information.  Stores names and addresses of all servers that are
 * to be queried for a zone transfer or SOA check of the desired zone. Normally
 * these are the authoritative primary and/or secondary nameservers for the
 * zone.
 */
char nsname[MAXNSNAME][MAXDNAME+1];		/* nameserver host name */
struct in_addr ipaddr[MAXNSNAME][MAXIPADDR];	/* nameserver addresses */
int naddrs[MAXNSNAME];				/* count of addresses */
int nservers = 0;				/* count of nameservers */

#ifdef notyet
/* more-structured  */
typedef struct srvr_data {
	char sd_nsname[MAXDNAME+1];		/* nameserver host name */
	struct in_addr sd_ipaddr[MAXIPADDR];	/* nameserver addresses */
	int sd_naddrs;				/* count of addresses */
} srvr_data_t;

srvr_data_t nsinfo[MAXNSNAME];	/* nameserver info */
unsigned int nsinfocount = 0;	/* count of nameservers */
#endif

bool_t authserver;		/* server is supposed to be authoritative */
bool_t lameserver;		/* server could not provide SOA service */

/*
 * Host information.
 * Stores names and (single) addresses encountered during the zone listing
 * of all A records that belong to the zone. Non-authoritative glue records
 * that do not belong to the zone are not stored. Glue records that belong
 * to a delegated zone will be filtered out later during the host count scan.
 * The host names are allocated dynamically.
 * The list itself is also allocated dynamically, to avoid static limits,
 * and to keep the initial bss of the executable to a reasonable size.
 * Allocation is done in chunks, to reduce considerable malloc overhead.
 * Note that the list will not shrink during recursive processing.
 */

#ifdef obsolete
char *hostname[MAXHOSTS];	/* host name of host in zone */
ipaddr_t hostaddr[MAXHOSTS];	/* first host address */
bool_t multaddr[MAXHOSTS];	/* set if this is a multiple address host */
#endif

typedef struct host_data {
	char *hd_hostname;	/* host name of host in zone */
	ipaddr_t hd_hostaddr;	/* first host address */
	bool_t hd_multaddr;	/* set if this is a multiple address host */
} host_data_t;

host_data_t *hostlist = NULL;	/* info on hosts in zone */
int hostcount = 0;		/* count of hosts in zone */

int maxhosts = 0;		/* number of allocated hostlist entries */

#define MAXHOSTINCR	4096	/* chunk size to increment hostlist */

#define hostname(i)	hostlist[i].hd_hostname
#define hostaddr(i)	hostlist[i].hd_hostaddr
#define multaddr(i)	hostlist[i].hd_multaddr

/*
 * Delegated zone information.
 * Stores the names of the delegated zones encountered during the zone
 * listing. The names and the list itself are allocated dynamically.
 */

char **zonename = NULL;		/* names of delegated zones within zone */
int zonecount = 0;		/* count of delegated zones within zone */

/*
 * Address information.
 * Stores the (single) addresses of hosts found in all zones traversed.
 * Used to search for duplicate hosts (same address but different name).
 * The list of addresses is allocated dynamically, and remains allocated.
 * This has now been implemented as a hashed list, using the low-order
 * address bits as the hash key.
 */

#ifdef obsolete
ipaddr_t *addrlist = NULL;	/* global list of addresses */
int addrcount = 0;		/* count of global addresses */
#endif

/*
 * SOA record information.
 */

soa_data_t soa;			/* buffer to store soa data */

int soacount = 0;		/* count of SOA records during listing */

/*
 * Nameserver preference.
 * As per BIND 4.9.* resource records may be returned after round-robin
 * reshuffling each time they are retrieved. For NS records, this may
 * lead to an unfavorable order for doing zone transfers.
 * We apply some heuristic to sort the NS records according to their
 * preference with respect to a given list of preferred server domains.
 */

int nsrank[MAXNSNAME];		/* nameserver ranking after sorting */

/*
** LIST_ZONE -- Basic routine to do complete zone listing and checking
** -------------------------------------------------------------------
**
**	Returns:
**		TRUE if the requested info was processed successfully.
**		FALSE otherwise.
*/

int total_calls = 0;		/* number of calls for zone processing */
int total_check = 0;		/* number of zones successfully processed */
int total_tries = 0;		/* number of zone transfer attempts */
int total_zones = 0;		/* number of successful zone transfers */
int total_hosts = 0;		/* number of hosts in all traversed zones */
int total_dupls = 0;		/* number of duplicates in all zones */

int zones_empty  = 0;		/* number of zones with zero hosts */
int zones_small  = 0;		/* number of zones with 1 - 9 hosts */
int zones_medium = 0;		/* number of zones with 10 - 99 hosts */
int zones_large  = 0;		/* number of zones with 100 - 999 hosts */
int zones_huge   = 0;		/* number of zones with 1000 or more hosts */

int hosts_empty  = 0;		/* number of hosts within empty zones (:-) */
int hosts_small  = 0;		/* number of hosts within small zones */
int hosts_medium = 0;		/* number of hosts within medium zones */
int hosts_large  = 0;		/* number of hosts within large zones */
int hosts_huge   = 0;		/* number of hosts within huge zones */

int total_stats[T_ANY+1];	/* total count of resource records per type */

#ifdef justfun
char longname[MAXDNAME+1];	/* longest host name found */
int longsize = 0;		/* size of longest host name */
#endif

bool_t
list_zone(name)
	input char *name;		/* name of zone to process */
{
	register int n;
	register int i;
	int nzones;			/* count of delegated zones */
	int nhosts;			/* count of real host names */
	int ndupls;			/* count of duplicate hosts */
	int nextrs;			/* count of extrazone hosts */
	int ngates;			/* count of gateway hosts */

	total_calls += 1;		/* update zone processing calls */

	/*
	 * Normalize to not have trailing dot, unless it is the root zone.
	 */
	n = strlength(name);
	if (n > 1 && name[n-1] == '.')
		name[n-1] = '\0';

	pr_timestamp("zone processing starting for %s", name);

	/*
	 * Indicate whether we are processing an in-addr.arpa reverse zone.
	 * In this case we will suppress accumulating host count statistics.
	 */
	reverse = indomain(name, ARPA_ROOT, FALSE);

	/*
	 * Enable various checks in certain circumstances.
	 * This affects processing in print_rrec(). It may need refinement.
	 */
#if 0 /* do it yourself!  --cnamecheck */
	if (addrmode && !reverse)
		cnamecheck = TRUE;
#endif

	/*
	 * Suppress various checks if working beyond the recursion skip level.
	 * This affects processing in print_rrec(). It may need refinement.
	 */
	if (!canonskip || canoncheck) {
		canonskip = ((recursion_level > skip_level) && !addrmode &&
			     !canoncheck) ? TRUE : FALSE;
	}

	underskip = ((recursion_level > skip_level) && !addrmode &&
		     !undercheck) ? TRUE : FALSE;

	/*
	 * Reset the load/dump switches for zone transfers to their defaults.
	 * These may be overruled hereafter, on a per-zone basis.
	 */
	dumping = dumpzone;	/* should dump to the cache */
	loading = loadzone;	/* should load from the cache */

	/*
	 * When not loading from the cache, compare the serial numbers in the
	 * cache and in reality, and avoid the transfer if nothing has changed.
	 * In that case, load the zone from the cache instead.
	 * Quit immediately if a quick dump of a single zone was requested.
	 */
	if (!loading && compare && compare_soa(name)) {
		if (verbose)
			printf("Avoiding zone transfer for %s\n", name);

		/* all done if just dumping a single zone */
		if (dumping && !recursive && quick)
			return ((errorcount == 0) ? TRUE : FALSE);

		/* load from the cache instead for further processing */
		dumping = FALSE;
		loading = TRUE;
	}

	/*
	 * Find the nameservers for the given zone.
	 * Make sure we have an address for at least one nameserver.
	 * We don't need the servers when loading the zone from the local cache,
	 * but we want them anyway if we are going to check the SOA records.
	 */
	if (!loading || checkmode) {
		(void) find_servers(name);	/* note we use nservers below to test success */

		if (nservers < 1) {
			errmsg("No nameservers for %s found", name);
			if (!loading)
				return (FALSE);
		}
		for (n = 0; n < nservers; n++) {
			if (naddrs[n] > 0)
				break;
		}
		if (nservers > 0 && n >= nservers) {
			errmsg("No addresses found for any of the nameservers for %s", name);
			if (!loading)
				return (FALSE);
		}
	}

	/*
	 * Without an explicit server on the command line, the servers we
	 * have looked up are supposed to be authoritative for the zone.
	 *
	 * XXX should we not set this if '-r' was specified (i.e. if recursion
	 * is already turned off in the resolver state)?
	 */
	authserver = (server && !primary) ? FALSE : TRUE;

	/*
	 * Check SOA records at each of the nameservers if so requested.
	 */
	if (checkmode) {
		do_check(name);

		total_check += 1;	/* update zones processed */

		/* all done if maximum recursion level reached */
		if (!recursive || (recursion_level >= recursive))
			return ((errorcount == 0) ? TRUE : FALSE);
	}

	/*
	 * The zone transfer for certain zones can be skipped.
	 */
	if (skip_transfer(name)) {
		if (verbose || statistics || checkmode || hostmode)
			printf("Skipping zone transfer for %s\n", name);
		return (FALSE);
	}

	/*
	 * Ask zone transfer to the nameservers, until one responds.
	 */
	pr_timestamp("zone transfer starting for %s", name);

	total_tries += 1;		/* update zone transfer attempts */

	if (!do_transfer(name))
		return (FALSE);

	total_zones += 1;		/* update successful zone transfers */

	pr_timestamp("zone transfer complete for %s", name);

	/*
	 * Print resource record statistics if so requested.
	 */
	if (statistics)
		print_stats(record_stats, 0, name, querytype, queryclass);

	/*
	 * Accumulate host count statistics for this zone.
	 * Do this only in modes in which such output would be printed.
	 */
	pr_timestamp("accumulate statistics for %s", name);

	nzones = zonecount;

	nhosts = 0, ndupls = 0, nextrs = 0, ngates = 0;

	i = ((verbose && !quick) || statistics || hostmode) ? 0 : hostcount;

	for (n = i; n < hostcount; n++) {
		/* skip fake hosts using a very rudimentary test */
		if (fakename(hostname(n)) || fakeaddr(hostaddr(n)))
			continue;
#ifdef justfun
		/* save longest host name encountered so far */
		if (verbose && ((i = strlength(hostname(n))) > longsize)) {
			longsize = i;
			(void) strcpy(longname, hostname(n));
		}
#endif
		/* skip apparent glue records */
		if (gluerecord(hostname(n), name, zonename, nzones)) {
			if (verbose > 1)
				printf("%s is a glue record\n", hostname(n));
			continue;
		}

		/* otherwise count as host */
		nhosts++;

		/*
		 * Mark hosts not residing directly in the zone as extrazone
		 * host.  These have extra label components without further
		 * delegation.
		 */
		if (!samedomain(hostname(n), name, TRUE)) {
			nextrs++;
			if (extrmode || (verbose > 1))
				printf("%s is an extrazone host\n", hostname(n));
		}

		/*
		 * Mark hosts with more than one address as gateway host.
		 * These are not checked for duplicate addresses.
		 */
		if (multaddr(n)) {
			ngates++;
			if (gatemode || (verbose > 1))
				printf("%s is a gateway host\n", hostname(n));
		} else if (check_dupl(hostaddr(n))) {
			/*
			 * Compare single address hosts against global list of
			 * addresses.  Multiple address hosts are too
			 * complicated to handle this way.
			 */
			struct in_addr inaddr;
			inaddr.s_addr = hostaddr(n);

			ndupls++;
			if (duplmode || (verbose > 1)) {
				printf("%s is a duplicate host, with address %s\n",
				       hostname(n), inet_ntoa(inaddr));
			}
		}
	}

	pr_timestamp("finished statistics for %s", name);

	/*
	 * Print statistics for this zone.
	 */
	if ((verbose && !quick) || statistics || hostmode) {
		printf("Encountered %d host%s within %s\n",
		       nhosts, plural(nhosts), name);

		if ((ndupls > 0) || duplmode || (verbose > 1)) {
			printf("Encountered %d duplicate host%s within %s\n",
			       ndupls, plural(ndupls), name);
		}
		if ((nextrs > 0) || extrmode || (verbose > 1)) {
			printf("Encountered %d extrazone host%s within %s\n",
			       nextrs, plural(nextrs), name);
		}
		if ((ngates > 0) || gatemode || (verbose > 1)) {
			printf("Encountered %d gateway host%s within %s\n",
			       ngates, plural(ngates), name);
		}
	}

	if (verbose || statistics) {
		printf("Found %d delegated zone%s within %s\n",
		       nzones, plural(nzones), name);
	}

	/*
	 * Update overall statistics.
	 */
	for (i = T_FIRST; i <= T_LAST; i++)
		total_stats[i] += record_stats[i];

	total_hosts += nhosts;		/* update total number of hosts */
	total_dupls += ndupls;		/* update total number of duplicates */

	if (nhosts < 1) {
		zones_empty += 1;
	} else if (nhosts < 10) {
		zones_small += 1;
		hosts_small += nhosts;
	} else if (nhosts < 100) {
		zones_medium += 1;
		hosts_medium += nhosts;
	} else if (nhosts < 1000) {
		zones_large += 1;
		hosts_large += nhosts;
	} else {
		zones_huge += 1;
		hosts_huge += nhosts;
	}

	if (!checkmode)
		total_check += 1;	/* update zones processed */

	/*
	 * Sort the encountered delegated zones alphabetically.
	 * Note that this precludes further use of the zone_index() function.
	 */
	pr_timestamp("sorting child zones for %s", name);

	if ((nzones > 1) && (recursive || listzones || mxdomains))
		qsort((ptr_t *) zonename, (size_t) nzones, sizeof(char *), compare_name);

	/*
	 * The names of the hosts were allocated dynamically.
	 */
	pr_timestamp("freeing host memory for %s", name);

	for (n = 0; n < hostcount; n++)
		xfree(hostname(n));

	/*
	 * Check for mailable delegated zones within this zone, based on
	 * ordinary MX lookup, not on the MX info in the zone listing, to
	 * reduce zone transfers.
	 */
	if (mxdomains) {
		if (recursion_level == 0) {
			if (verbose)
				printf("\n");

			if (!get_mxrec(name))
				ns_error(name, T_MX, queryclass, server);
		}
		for (n = 0; n < nzones; n++) {
			if (verbose)
				printf("\n");

			if (!get_mxrec(zonename[n]))
				ns_error(zonename[n], T_MX, queryclass, server);
		}
	}

	/*
	 * Do recursion on delegated zones if requested and any were found.
	 * Temporarily save zonename list, and force allocation of new list.
	 */
	if (recursive && (recursion_level < recursive)) {
		for (n = 0; n < nzones; n++) {
			char **newzone;		/* local copy of list */

			newzone = zonename;
			zonename = NULL;	/* allocate new list */

			if (verbose || statistics || checkmode || hostmode)
				printf("\n");

			if (listzones) {
				for (i = 0; i <= recursion_level; i++)
					printf("%s", (i == 0) ? "\t" : "  ");
				printf("%s\n", newzone[n]);
			}

			if (verbose)
				printf("Entering zone %s\n", newzone[n]);

			recursion_level++;
			(void) list_zone(newzone[n]);
			recursion_level--;

			zonename = newzone;	/* restore */
		}
	} else {
		if (listzones) {
			for (n = 0; n < nzones; n++) {
				for (i = 0; i <= recursion_level; i++)
					printf("%s", (i == 0) ? "\t" : "  ");
				printf("%s\n", zonename[n]);
			}
		}
	}

	/*
	 * The names of the delegated zones were allocated dynamically.
	 * The list of delegated zone names was also allocated dynamically.
	 */
	pr_timestamp("freeing zone memory for %s", name);

	for (n = 0; n < nzones; n++)
		xfree(zonename[n]);

	if (zonename != NULL)
		xfree(zonename);

	zonename = NULL;

	/*
	 * Print final overall statistics.
	 */
	if (recursive && (recursion_level == 0) && nzones > 0) {
		if (verbose || statistics || checkmode || hostmode)
			printf("\nCumulative Totals:\n");

		if (statistics)
			print_stats(total_stats, total_zones, name, querytype, queryclass);

		if ((verbose && !quick) || statistics || hostmode) {
			printf("Encountered %d host%s in %d zone%s within %s\n",
			       total_hosts, plural(total_hosts),
			       total_zones, plural(total_zones),
			       name);
		}
		if ((verbose && !quick) || statistics || hostmode) {
			printf("Encountered %d duplicate host%s in %d zone%s within %s\n",
			       total_dupls, plural(total_dupls),
			       total_zones, plural(total_zones),
			       name);
		}
		if (verbose || statistics || checkmode) {
			printf("Transferred %d zone%s out of %d attempt%s\n",
			       total_zones, plural(total_zones),
			       total_tries, plural(total_tries));
		}
		if (verbose || statistics || checkmode) {
			printf("Processed %d zone%s out of %d request%s\n",
			       total_check, plural(total_check),
			       total_calls, plural(total_calls));
		}
#ifdef justfun
		if (verbose && (longsize > 0)) {
			printf("Longest hostname %s\t%d\n",
			       longname, longsize);
		}
#endif
		if ((verbose && !quick) || statistics || hostmode) {
			if (zones_empty > 0) {
				printf("Classified %d/%d empty zone%s (%d/%d host%s) within %s\n",
				       zones_empty, total_zones, plural(zones_empty),
				       hosts_empty, total_hosts, plural(hosts_empty),
				       name);
			}
			if (zones_small > 0) {
				printf("Classified %d/%d small zone%s (%d/%d host%s) within %s\n",
				       zones_small, total_zones, plural(zones_small),
				       hosts_small, total_hosts, plural(hosts_small),
				       name);
			}
			if (zones_medium > 0) {
				printf("Classified %d/%d medium zone%s (%d/%d host%s) within %s\n",
				       zones_medium, total_zones, plural(zones_medium),
				       hosts_medium, total_hosts, plural(hosts_medium),
				       name);
			}
			if (zones_large > 0) {
				printf("Classified %d/%d large zone%s (%d/%d host%s) within %s\n",
				       zones_large, total_zones, plural(zones_large),
				       hosts_large, total_hosts, plural(hosts_large),
				       name);
			}
			if (zones_huge > 0) {
				printf("Classified %d/%d huge zone%s (%d/%d host%s) within %s\n",
				       zones_huge, total_zones, plural(zones_huge),
				       hosts_huge, total_hosts, plural(hosts_huge),
				       name);
			}
		}
	}

	pr_timestamp("zone processing complete for %s", name);

	/* indicate whether any errors were encountered */
	return ((errorcount == 0) ? TRUE : FALSE);
}


/*
** USE_SERVERS -- prepare to use the first MAXNS addresses of the auth servers
** ---------------------------------------------------------------------------
**
**	Returns:
**		TRUE if servers could be determined successfully.
**		FALSE otherwise.
**
**	Outputs:
**		The resolver is initialised to use the discovered servers.
**
** XXX should maybe use first address of each server, then if MAXNS not yet
** reached the next address of each server, and so on....
*/

bool_t
use_servers(name)
	input char *name;
{
	register int n, i;

	if (find_servers(name) == FALSE || nservers < 1) {
		errmsg("No nameservers for %s found", name);
		return (FALSE);
	}
	/* reset list of nameservers to none */
	_res.nscount = 0;

	/* load the nameserver addresses into nslist */
	for (n = 0; n < nservers; n++) {
		if (naddrs[n] < 1)
			continue;	/* shortcut */
		server = nsname[n];	/* BOGUS!  but needed to enable global state */
		for (i = 0; _res.nscount < MAXNS && i < naddrs[n]; i++, _res.nscount++) {
			nslist(_res.nscount).sin_family = AF_INET;
			nslist(_res.nscount).sin_port = htons(NAMESERVER_PORT);
			nslist(_res.nscount).sin_addr = ipaddr[n][i];
		}
	}
	return (TRUE);
}

/*
** FIND_SERVERS -- Fetch names and addresses of authoritative servers
** ------------------------------------------------------------------
**
**	Returns:
**		TRUE if servers could be determined successfully.
**		FALSE otherwise.
**
**	Inputs:
**		The global variable ``server'', if set, contains the
**		name of the explicit server to be contacted.
**		The global variable ``primary'', if set, indicates
**		that we must use the primary nameserver for the zone.
**		If both are set simultaneously, the explicit server
**		is contacted to retrieve the desired servers.
**
**	Outputs:
**		The count of nameservers is stored in ``nservers''.
**		Names are stored in the nsname[] database.
**		Addresses are stored in the ipaddr[] database.
**		Address counts are stored in the naddrs[] database.
*/

bool_t
find_servers(name)
	input char *name;		/* name of zone to find servers for */
{
	struct hostent *hp;
	register int n, i;

	/*
	 * Use the explicit server if given on the command line.  Its addresses
	 * will have already been loaded into the resolver library's nslist
	 * vector by the call to set_server() from main().
	 *
	 * This server may not be authoritative for the given zone.
	 */
	if (server && !primary) {
		(void) strcpy(nsname[0], server);

		for (i = 0; i < MAXIPADDR && i < _res.nscount; i++)
			ipaddr[0][i] = nslist(i).sin_addr;
		naddrs[0] = i;

		nservers = 1;
		return (TRUE);
	}

	/*
	 * Fetch primary nameserver info if so requested.
	 * 
	 * Get its name from the SOA record for the zone, and do a regular
	 * host lookup to fetch its addresses. We are assuming here that the
	 * SOA record is a proper one. This is not necessarily true.
	 * Obviously this server should be authoritative.
	 */
	if (primary && !server) {
		char *primaryname;

		if (!(primaryname = get_primary(name))) {
			ns_error(name, T_SOA, queryclass, server);
			nservers = 0;
			return (FALSE);
		}

		if (!(hp = geth_byname(primaryname))) {
			ns_error(primaryname, T_A, C_IN, server);
			nservers = 0;
			return (FALSE);
		}

		primaryname = strncpy(nsname[0], hp->h_name, (size_t) MAXDNAME);
		primaryname[MAXDNAME] = '\0';

		for (i = 0; i < MAXIPADDR && hp->h_addr_list[i]; i++)
			ipaddr[0][i] = incopy(hp->h_addr_list[i]);
		naddrs[0] = i;

		if (verbose)
			printf("Found %d address%s for primary %s\n",
				naddrs[0], plurale(naddrs[0]), nsname[0]);
		nservers = 1;
		geth_freehostent(hp);

		return (TRUE);
	}

	/*
	 * Otherwise we have to find the nameservers for the zone.
	 * These are supposed to be authoritative, but sometimes we
	 * encounter lame delegations, perhaps due to misconfiguration.
	 */
	if (!get_servers(name)) {
		ns_error(name, T_NS, queryclass, server);
		nservers = 0;
		return (FALSE);
	}

	/*
	 * Usually we'll get addresses for all the servers in the additional
	 * info section.  But in case we don't, look up their addresses.
	 * Addresses could be missing because there is no room in the answer.
	 * No address is present if the name of a server is not canonical.
	 * If we get no addresses by extra query, and this is authoritative,
	 * we flag a lame delegation to that server.
	 */
	for (n = 0; n < nservers; n++) {
		if (naddrs[n] == 0) {
			if ((hp = geth_byname(nsname[n]))) {
				for (i = 0; i < MAXIPADDR && hp->h_addr_list[i]; i++)
					ipaddr[n][i] = incopy(hp->h_addr_list[i]);
				naddrs[n] = i;
			}
			if (verbose > 1) {
				printf("Found %d address%s for %s by extra query\n",
				       naddrs[n], plurale(naddrs[n]), nsname[n]);
			}
			if (!hp) {
				/* server name lookup failed */
				ns_error(nsname[n], T_A, C_IN, server);

				/* authoritative denial: probably misconfiguration */
				if (h_errno == NO_DATA || h_errno == HOST_NOT_FOUND) {
					if (server == NULL) {
						pr_error("No adddress for NS!  %s has lame delegation to %s",
							 name, nsname[n]);
					}
				}
			}
			if ((hp != NULL) && !sameword(hp->h_name, nsname[n])) {
				pr_warning("%s nameserver %s is not canonical (%s)",
					   name, nsname[n], hp->h_name);
			}
			if (hp)
				geth_freehostent(hp);
		} else {
			if (verbose > 1) {
				printf("Found %d address%s for %s\n",
				       naddrs[n], plurale(naddrs[n]), nsname[n]);
			}
		}
	}

	/*
	 * Issue warning if only one server has been discovered.
	 * This is not an error per se, but not much redundancy in that case.
	 */
	if (nservers == 1) {
		pr_warning("%s has only one nameserver %s",
			   name, nsname[0]);
	}

	return ((nservers > 0) ? TRUE : FALSE);
}

/*
** GET_SERVERS -- Fetch names and addresses of authoritative servers
** -----------------------------------------------------------------
**
**	Returns:
**		TRUE if servers could be determined successfully.
**		FALSE otherwise.
**
**	Side effects:
**		The count of nameservers is stored in ``nservers''.
**		Names are stored in the nsname[] database.
**		Addresses are stored in the ipaddr[] database.
**		Address counts are stored in the naddrs[] database.
*/

bool_t
get_servers(name)
	input char *name;		/* name of zone to find servers for */
{
	querybuf_t answer;
	register int n;
	res_state_t save_res;		/* saved copy of resolver database */

	if (verbose)
		printf("Finding nameservers for %s ...\n", name);

	/*
	 * honour --parent by using the parent zone's nameservers to search
	 * (non-recursively) for the nameservers of the zone to be checked.
	 *
	 * We need to use the parent zone's nameservers only for the
	 * get_info(T_NS) call below, and we only want recursion turned off
	 * when looking up the NS records, but not their corresponding A
	 * records.
	 */
	if (parent) {
		/*
		 * XXX this code should be refactored out into a separate
		 * function so that the similar code in main.c:execute()
		 * can also be refactored out and use the same function.
		 */
		char pzn[MAXDNAME + 2];
		char *parent_zone;
		int save_verbose;

		/*
		 * save resolver database so that we can restore it when we go
		 * to find the A RRs for the discovered NS RRs
		 */
		save_res = _res;
		server = NULL;

		_res.options &= ~RES_RECURSE;

		/* there is always one final parent:  "." */
		assert(strlen(name) <= MAXDNAME);
		sprintf(pzn, "%s.", name);
		parent_zone = pzn;

		if (verbose > 1)
			printf("(Attempting to use the parent nameservers for %s)\n", name);

		/* use_servers() will recursively call back here! */
		parent = FALSE;

		save_verbose = verbose;
		if (verbose)
			verbose--;

		while ((parent_zone = strchr(parent_zone, '.'))) {
			size_t dot_offset = 0;

			/* skip the dot unless it's the last dot */
			parent_zone = *(parent_zone + 1) ? (parent_zone + 1) : parent_zone;
			/* trim the trailing dot unless it's the last one */
			if (*(parent_zone + 1)) {
				dot_offset = strlen(parent_zone) - 1;
				parent_zone[dot_offset] = '\0';
			}
			if (save_verbose)
				printf("(Attempting to find nameservers for the parent zone: %s)\n", parent_zone);
			if (use_servers(parent_zone) != FALSE) {
				parent = TRUE; /* used to indicate success */
				break;
			}
			/* make sure next call to use_servers works right! */
			server = NULL;
			/* restore any trimmed trailing dot in case it's the last one */
			if (dot_offset)
				parent_zone[dot_offset] = '.';
		}
		verbose = save_verbose;
		if (!parent) {
			ns_error(name, T_NS, queryclass, (char *) NULL);
			_res = save_res;
			return (FALSE);
		}
		if (verbose > 1 || debug > 1)
			show_res();
	}

	if ((n = get_info(&answer, name, T_NS, queryclass)) < 0)
		return (FALSE);

	if (parent) {
		/*
		 * restore resolver database so that the code in find_servers()
		 * which searches for A RRs for the found nameservers can use
		 * the local cache
		 */
		_res = save_res;
		/* make it look like the user did not explicitly specify a server */
		server = NULL;
		/* turn on recursion again if it wasn't explicitly disabled */
		if (! norecurs)
			_res.options |= RES_RECURSE;
	}

	if (verbose > 1)
		(void) print_info(&answer, n, name, T_NS, queryclass, FALSE);

	return get_nsinfo(&answer, n, name, T_NS, queryclass);
}

/*
** GET_NSINFO -- Extract nameserver data from nameserver answer buffer
** -------------------------------------------------------------------
**
**	Returns:
**		TRUE if the answer buffer was processed successfully.
**		FALSE otherwise.
**
**	Outputs:
**		The count of nameservers is stored in ``nservers''.
**		Names are stored in the nsname[] database.
**		Addresses are stored in the ipaddr[] database.
**		Address counts are stored in the naddrs[] database.
*/

bool_t
get_nsinfo(answerbuf, answerlen, name, qtype, qclass)
	input querybuf_t *answerbuf;	/* location of answer buffer */
	input int answerlen;		/* length of answer buffer */
	input char *name;		/* name of zone to find servers for */
	input int qtype;		/* record type we are querying about */
	input int qclass;		/* record class we are querying about */
{
	HEADER *bp;
	int qdcount, ancount, nscount, arcount, rrcount;
	u_char *msg, *eom;
	register u_char *cp;
	register int i;

	nservers = 0;			/* count of nameservers */

	bp = (HEADER *) answerbuf;
	qdcount = ntohs((u_short) bp->qdcount);
	ancount = ntohs((u_short) bp->ancount);
	nscount = ntohs((u_short) bp->nscount);
	arcount = ntohs((u_short) bp->arcount);

	msg = (u_char *) answerbuf;
	eom = (u_char *) answerbuf + answerlen;
	cp  = (u_char *) answerbuf + HFIXEDSZ;

	if (qdcount > 0 && cp < eom) {	/* should be exactly one record */
		if (!(cp = skip_qrec(name, qtype, qclass, cp, msg, eom)))
			return (FALSE);
		qdcount--;
	}
	if (qdcount) {
		pr_error("invalid qdcount after %s query for %s",
			 pr_type(qtype), name);
		set_h_errno(NO_RECOVERY);
		return (FALSE);
	}

	/*
	 * If the answer is authoritative, the names are found in the
	 * answer section, and the nameserver section is empty.
	 * If not, there may be duplicate names in both sections.
	 * Addresses are found in the additional info section both cases.
	 */
	rrcount = ancount + nscount + arcount;
	while (rrcount > 0 && cp < eom) {
		char rname[MAXDNAME+1];
		char dname[MAXDNAME+1];
		int type, class, ttl, dlen;
		u_char *eor;
		register int n;
		struct in_addr inaddr;

		if ((n = expand_name(name, T_NONE, cp, msg, eom, rname)) < 0)
			return (FALSE);
		cp += n;

		n = (3 * INT16SZ) + INT32SZ;
		if (check_size(rname, T_NONE, cp, msg, eom, n) < 0)
			return (FALSE);

		type = ns_get16(cp);
		cp += INT16SZ;

		class = ns_get16(cp);
		cp += INT16SZ;

		ttl = ns_get32(cp);
		cp += INT32SZ;

		dlen = ns_get16(cp);
		cp += INT16SZ;

		if (check_size(rname, type, cp, msg, eom, dlen) < 0)
			return (FALSE);
		eor = cp + dlen;
#ifdef DEBUG
		if (verbose > 2) {
			printf("get_nsinfo(): DEBUG: %-20s\t%d\t%s\t%s\n",
			       rname, ttl, pr_class(class), pr_type(type));
		}
#endif
		if ((type == T_NS) && sameword(rname, name)) {
			if ((n = expand_name(rname, type, cp, msg, eom, dname)) < 0)
				return (FALSE);
			cp += n;

			for (i = 0; i < nservers; i++) {
				if (sameword(nsname[i], dname))
					break;	/* duplicate */
			}
			if (i >= nservers && nservers < MAXNSNAME) {
				(void) strcpy(nsname[nservers], dname);
				naddrs[nservers] = 0;
				nservers++;
			}
		} else if ((type == T_A) && (dlen == INADDRSZ)) {
			for (i = 0; i < nservers; i++) {
				if (sameword(nsname[i], rname))
					break;	/* found */
			}
			if (i < nservers && naddrs[i] < MAXIPADDR) {
				memcpy((char *) &inaddr, (char *) cp, (size_t) INADDRSZ);
				ipaddr[i][naddrs[i]] = inaddr;
				naddrs[i]++;
			}
			cp += dlen;
		} else {
			/* just ignore other records */
			cp += dlen;
		}
		if (cp != eor) {
			pr_error("size error in %s record for %s, off by %s",
				 pr_type(type), rname, dtoa((int) (cp - eor)));
			set_h_errno(NO_RECOVERY);
			return (FALSE);
		}
		rrcount--;
	}
	if (rrcount) {
		pr_error("invalid rrcount after %s query for %s",
			 pr_type(qtype), name);
		set_h_errno(NO_RECOVERY);
		return (FALSE);
	}

	/* set proper status if no answers found */
	set_h_errno((nservers > 0) ? 0 : TRY_AGAIN);

	return (TRUE);
}

/*
** SORT_SERVERS -- Sort set of nameservers according to preference
** ---------------------------------------------------------------
**
**	XXX This is probably a waste of time.  The only thing that really makes
**	sense here is to prefer NS target host addresses that are on the same
**	network as some interface address and to leave the rest the way they
**	came.
**
**	Returns:
**		None.
**
**	Inputs:
**		Set of nameservers as determined by find_servers() and stored
**		in the global variable ``nsname[]''.
**
**		The global variable ``prefserver'', if set, contains a list of
**		preferred server domains to compare against so priority for
**		zone transfers can be given to to "preferred" servers,
**		i.e. those residing in domains given.  The more domain
**		component labels match, the higher the priority.  By default
**		priority is given to servers within your own domain or parent
**		domains under the assumption that such servers will either be
**		closer in the network topology or more likely to allow
**		transfers, or both, since the order in which NS records are
**		found in an NS answer may be random.
**
**	Side effects:
**		Stores the preferred nameserver order in the global variable
**		``nsrank[]''.
*/

void
sort_servers()
{
	register int i, j;
	register int n, pref;
	register char *p, *q;
	int nspref[MAXNSNAME];		/* nameserver preference value */

	/*
	 * Initialize the default ranking.
	 */
	for (n = 0; n < nservers; n++) {
		nsrank[n] = n;
		nspref[n] = 0;
	}

	if (!prefserver)
		return;			/* leave everything as-is */

	/*
	 * Determine the nameserver preference.
	 * Compare against a list of comma-separated preferred server domains.
	 * Use the maximum value of all comparisons.
	 */
	for (q = prefserver, p = q; p != NULL; p = q) {
		if ((q = strchr(p, ',')))
			*q = '\0';

		for (n = 0; n < nservers; n++) {
			pref = matchlabels(nsname[n], p);
			if (pref > nspref[n])
				nspref[n] = pref;
		}
		if (q)
			*q++ = ',';
	}

	/*
	 * Sort the set according to preference.
	 * Keep the rest as much as possible in original order.
	 */
	for (i = 0; i < nservers; i++) {
		for (j = i + 1; j < nservers; j++) {
			if (nspref[j] > nspref[i]) {
				pref = nspref[j];
				/* nspref[j] = nspref[i]; */
				for (n = j; n > i; n--)
					nspref[n] = nspref[n-1];
				nspref[i] = pref;

				pref = nsrank[j];
				/* nsrank[j] = nsrank[i]; */
				for (n = j; n > i; n--)
					nsrank[n] = nsrank[n-1];
				nsrank[i] = pref;
			}
		}
	}

	return;
}

/*
** SKIP_TRANSFER -- Check whether a zone transfer should be skipped
** ----------------------------------------------------------------
**
**	Returns:
**		TRUE if a transfer for this zone should be skipped.
**		FALSE if the zone transfer should proceed.
**
**	Inputs:
**		The global variable ``skipzone'', if set, contains
**		a list of zone names to be skipped.
**
**	Certain zones are known to contain bogus information, and
**	can be requested to be excluded from further processing.
**	The zone transfer for such zones and its delegated zones
**	will be skipped.
*/

bool_t
skip_transfer(name)
	input char *name;			/* name of zone to process */
{
	register char *p, *q;
	bool_t skip = FALSE;

	for (q = skipzone, p = q; p != NULL; p = q) {
		if ((q = strchr(p, ',')))
			*q = '\0';

		if (sameword(name, p))
			skip = TRUE;

		if (q)
			*q++ = ',';
	}

	return (skip);
}

/*
** DO_CHECK -- Check SOA records at each of the nameservers
** --------------------------------------------------------
**
**	Returns:
**		None.
**
**	Inputs:
**		The count of nameservers is stored in ``nservers''.
**		Names are stored in the nsname[] database.
**		Addresses are stored in the ipaddr[] database.
**		Address counts are stored in the naddrs[] database.
**
**	The SOA record of the zone is checked at each nameserver.
**	Nameserver recursion is turned off to make sure that the
**	answer is authoritative.
*/

void
do_check(name)
	input char *name;		/* name of zone to process */
{
	res_state_t save_res;		/* saved copy of resolver database */
	char *save_server;		/* saved copy of server name */
	register int n;
	register int i;

	/*
	 * First check the local cache, if appropriate.
	 */
	if (loading && !check_cache(name, "cache")) {
		/* SOA query failed */
		ns_error(name, T_SOA, queryclass, "cache");
	}

	/*
	 * Then continue with each of the nameservers.
	 */
	/* save resolver database */
	save_res = _res;
	save_server = server;

	/*
	 * turn off nameserver recursion
	 *
	 * if authserver is set then each _MUST_ be authoritative, and if
	 * authserver is not set then it is likely the user only wants to check
	 * what the specified server already knows, not ask it to first fetch
	 * the record.
	 */
	_res.options &= ~RES_RECURSE;

	for (n = 0; n < nservers; n++) {
		if (naddrs[n] < 1)
			continue;	/* shortcut */

		server = nsname[n];

		/* XXX could use just one index -- both start at zero */
		for (i = 0, _res.nscount = 0; _res.nscount < MAXNS && i < naddrs[n]; i++, _res.nscount++) {
			nslist(_res.nscount).sin_family = AF_INET;
			nslist(_res.nscount).sin_port = htons(NAMESERVER_PORT);
			nslist(_res.nscount).sin_addr = ipaddr[n][i];
		}

		/* retrieve and check SOA */
		if (check_zone(name, server))
			continue;

		/* SOA query failed */
		ns_error(name, T_SOA, queryclass, server);

		/* explicit server failure: possibly data expired */
		lameserver = (h_errno == SERVER_FAILURE) ? TRUE : FALSE;

		/* non-authoritative denial: assume lame delegation */
		if (h_errno == NO_RREC || h_errno == NO_HOST)
			lameserver = TRUE;

		/* authoritative denial: probably misconfiguration */
		if (h_errno == NO_DATA || h_errno == HOST_NOT_FOUND)
			lameserver = TRUE;

		/* flag an error if server should not have failed */
		if (lameserver && authserver) {
			pr_error("%s has lame delegation to the nameserver %s",
				 name, server);
		} else if (lameserver) {
			pr_warning("%s no SOA found at the specified nameserver %s",
				   name, server);
		}
	}

	/* restore resolver database */
	_res = save_res;
	server = save_server;

	return;
}

/*
** DO_SOA -- Check SOA record at a single nameserver address
** ---------------------------------------------------------
**
**	Returns:
**		None.
**
**	The zone SOA record is checked at one nameserver address.
**	Nameserver recursion is turned off to make sure that the
**	answer is authoritative.
*/

void
do_soa(name, inaddr, host)
	input char *name;		/* name of zone to process */
	input struct in_addr inaddr;	/* address of server to be queried */
	input char *host;		/* name of server to be queried */
{
	res_state_t save_res;		/* saved copy of resolver database */
	char *save_server;		/* saved copy of server name */
	querybuf_t answer;
	HEADER *bp;
	register int n;

	/* save resolver database */
	save_res = _res;
	save_server = server;

	/* turn off nameserver recursion */
	_res.options &= ~RES_RECURSE;

	/* substitute explicit server name and address */
	server = host;
	nslist(0).sin_family = AF_INET;
	nslist(0).sin_port = htons(NAMESERVER_PORT);
	nslist(0).sin_addr = inaddr;
	_res.nscount = 1;

	if (verbose)
		printf("Asking SOA record for %s ...\n", name);

	if ((n = get_info(&answer, name, T_SOA, queryclass)) < 0) {
		/* SOA query failed */
		ns_error(name, T_SOA, queryclass, server);

		/* explicit server failure: possibly data expired */
		lameserver = (h_errno == SERVER_FAILURE) ? TRUE : FALSE;

		/* non-authoritative denial: assume lame delegation */
		if (h_errno == NO_RREC || h_errno == NO_HOST)
			lameserver = TRUE;

		/* authoritative denial: probably misconfiguration */
		if (h_errno == NO_DATA || h_errno == HOST_NOT_FOUND)
			lameserver = TRUE;

		/* flag an error if server should not have failed */
		if (lameserver && authserver) {
			pr_error("No SOA.  %s has lame delegation to %s",
				 name, server);
		} else if (lameserver) {
			pr_warning("No SOA found at server %s",
				   name, server);
		}
	}
	bp = (HEADER *) &answer;
	if ((n > 0) && !bp->aa) {
		if (authserver) {
			pr_error("Non-authoritative SOA!  %s has lame delegation to server %s",
				 name, server);
		} else {
			pr_warning("%s SOA record at %s is not authoritative",
				   name, server);
		}
	}

	/* restore resolver database */
	_res = save_res;
	server = save_server;

	return;
}

/*
** DO_TRANSFER -- Perform a zone transfer from any of its nameservers
** ------------------------------------------------------------------
**
**	Returns:
**		TRUE if the zone data have been retrieved successfully.
**		FALSE if none of the servers responded.
**
**	Inputs:
**		The count of nameservers is stored in ``nservers''.
**		Names are stored in the nsname[] database.
**		Addresses are stored in the ipaddr[] database.
**		Address counts are stored in the naddrs[] database.
*/

bool_t
do_transfer(name)
	input char *name;		/* name of zone to do zone xfer for */
{
	register int n, ns;
	register int i;

	/*
	 * When loading the zone from the local cache, just go ahead.
	 */
	if (loading) {
		static struct in_addr inaddr;	/* unused */

		if (transfer_zone(name, inaddr, "cache"))
			return (TRUE);

		ns_error(name, T_AXFR, queryclass, "cache");
		return (FALSE);
	}

	/*
	 * Ask zone transfer to the nameservers, until one responds.
	 * The list of nameservers is sorted according to preference.
	 * An authoritative server should always respond positively.
	 * If it responds with an error, we may have a lame delegation.
	 * Always retry with the next server to avoid missing entire zones.
	 */
	for (sort_servers(), ns = 0; ns < nservers; ns++) {
		for (n = nsrank[ns], i = 0; i < naddrs[n]; i++) {
			if (verbose) {
				printf("Trying server %s (%s) ...\n",
				       inet_ntoa(ipaddr[n][i]), nsname[n]);
			}
			if (transfer_zone(name, ipaddr[n][i], nsname[n]))
				return (TRUE);

			/* terminate on cache I/O errors */
			if (h_errno == CACHE_ERROR) {
				errmsg("No cache for %s created", name);
				return (FALSE);
			}

			/* zone transfer failed */
			if ((h_errno != TRY_AGAIN) || verbose)
				ns_error(name, T_AXFR, queryclass, nsname[n]);

			/* zone transfer request was explicitly refused */
			if (h_errno == QUERY_REFUSED) {
				do_soa(name, ipaddr[n][i], nsname[n]);
				set_h_errno(QUERY_REFUSED);
				break;
			}

			/* explicit server failure: possibly data expired */
			lameserver = (h_errno == SERVER_FAILURE) ? TRUE : FALSE;

			/* non-authoritative denial: assume lame delegation */
			if (h_errno == NO_RREC || h_errno == NO_HOST)
				lameserver = TRUE;

			/* authoritative denial: probably misconfiguration */
			if (h_errno == NO_DATA || h_errno == HOST_NOT_FOUND)
				lameserver = TRUE;

			/* flag an error if server should not have failed */
			if (lameserver && authserver) {
				pr_error("AXFR failed: %s has lame delegation to %s",
					 name, nsname[n]);
			} else if (lameserver) {
				pr_warning("AXFR failed: %s is not authoritative for the zone %s",
					   nsname[n], name);
			}

			/* try next server if this one is sick */
			if (lameserver)
				break;

			/* terminate on irrecoverable errors */
			if (h_errno != TRY_AGAIN)
				return (FALSE);

			/* in case nameserver not present */
			if (errno == ECONNREFUSED)
				break;
		}
	}

	if (nservers > 0 && ns >= nservers) {
		if ((h_errno == TRY_AGAIN) && !verbose)
			ns_error(name, T_AXFR, queryclass, (char *) NULL);
	}

	errmsg("No nameservers for %s responded", name);

	return (FALSE);
}

/*
** TRANSFER_ZONE -- Wrapper for get_zone() to hide administrative tasks
** --------------------------------------------------------------------
**
**	Returns:
**		See get_zone() for details.
**
**	Side effects:
**		See get_zone() for details.
**
**	This routine may be called repeatedly with different server
**	addresses, until one of the servers responds. Various items
**	must be reset on every try to continue with a clean slate.
*/

bool_t
transfer_zone(name, inaddr, host)
	input char *name;		/* name of zone to do zone xfer for */
	input struct in_addr inaddr;	/* address of server to be queried */
	input char *host;		/* name of server to be queried */
{
	bool_t result;
	register int n;

	/*
	 * Reset the resource record statistics before each try.
	 */
	clear_stats(record_stats);

	/*
	 * Reset the hash tables of saved resource record information.
	 * These tables are used only during the zone transfer itself.
	 * The zonetab is now also used when filtering glue records afterwards.
	 */
	clear_ttltab();
	clear_hosttab();
	clear_zonetab();

	/*
	 * Create temporary cache file if data must be dumped.
	 * In case this fails, the entire zone transfer is cancelled.
	 */
	if (dumping && (cache_open(name, TRUE) < 0)) {
		set_h_errno(CACHE_ERROR);
		return (FALSE);
	}

	/*
	 * Perform the actual zone transfer.
	 * All error reporting is done by get_zone().
	 */
	result = get_zone(name, inaddr, host);

	/*
	 * Move temporary cache file to real cache file in case the transfer
	 * was successful. Otherwise just delete the temporary cache file.
	 * If the cache cannot be created, the transfer is marked to have failed.
	 */
	if (dumping && (cache_close(result) < 0)) {
		set_h_errno(CACHE_ERROR);
		result = FALSE;
	}

	/*
	 * On failure to get the zone, free any memory that may have been allocated.
	 * On success it is the responsibility of the caller to free the memory.
	 * The information gathered is used by list_zone() after the zone transfer.
	 */
	if (!result) {
		for (n = 0; n < hostcount; n++)
			xfree(hostname(n));

		for (n = 0; n < zonecount; n++)
			xfree(zonename[n]);

		if (zonename != NULL)
			xfree(zonename);

		zonename = NULL;
	}

	return (result);
}

/*
** GET_ZONE -- Perform a zone transfer from server at specific address
** -------------------------------------------------------------------
**
**	Returns:
**		TRUE if the zone data have been retrieved successfully.
**		FALSE if an error occurred (h_errno is set appropriately).
**		Set TRY_AGAIN wherever possible to try the next server.
**
**	Side effects:
**		Stores list of delegated zones found in zonename[],
**		and the count of delegated zones in ``zonecount''.
**		Stores list of host names found in hostname[],
**		and the count of host names in ``hostcount''.
**		Updates resource record statistics in record_stats[].
**		This array must have been cleared before.
*/

bool_t
get_zone(name, inaddr, host)
	input char *name;		/* name of zone to do zone xfer for */
	input struct in_addr inaddr;	/* address of server to be queried */
	input char *host;		/* name of server to be queried */
{
	querybuf_t query;
	char *answer = NULL;		/* allocated to size -- axfer replies can be big! */
	HEADER *bp;
	int ancount;
	int sock = -1;
	struct sockaddr_in ns_sin;
	register int i;
	register int n;
	int nrecords = 0;		/* number of records processed */
	int npackets = 0;		/* number of packets received */
	int len;

	/* clear global counts */
	soacount = 0;			/* count of SOA records */
	zonecount = 0;			/* count of delegated zones */
	hostcount = 0;			/* count of host names */

	set_errno(0);	/* reset before going on any furhter */

	/*
	 * When loading the zone from the local cache, the cache file must exist.
	 */
	if (loading) {
		/*
		 * open the cache file for reading....
		 */
		if (cache_open(name, FALSE) < 0) {
			set_h_errno(NO_RREC);
			return (FALSE);
		}

		if (verbose)
			printf("Loading zone from cache for %s ...\n", name);
	} else {
		/*
		 * Construct query, and connect to the given server.
		 */
		n = res_mkquery(QUERY, name, queryclass, T_AXFR, (qbuf_t *) NULL, 0,
				(rrec_t *) NULL, (qbuf_t *) &query, (int) sizeof(querybuf_t));
		if (n < 0) {
			if (debug)
				printf("%sres_mkquery failed\n", debug_prefix);
			set_h_errno(NO_RECOVERY);
			return (FALSE);
		}

		if (debug) {
			printf("%sget_zone()\n", debug_prefix);
			pr_query((qbuf_t *) &query, n, stdout);
		}

		/* setup destination address */
		memset((char *) &ns_sin, (int) '\0', sizeof(ns_sin));

		ns_sin.sin_family = AF_INET;
		ns_sin.sin_port = htons(NAMESERVER_PORT);
		ns_sin.sin_addr = inaddr;

		sock = host_res_socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			host_res_perror(&ns_sin, host, "socket()");
			set_h_errno(TRY_AGAIN);
			return (FALSE);
		}

		if (host_res_connect(sock, &ns_sin, (socklen_t) sizeof(ns_sin)) < 0) {
			if (verbose || debug)
				host_res_perror(&ns_sin, host, "connect()");
			(void) host_res_close(sock);
			set_h_errno(TRY_AGAIN);
			return (FALSE);
		}

		if (verbose)
			printf("Asking zone transfer for %s ...\n", name);

		/*
		 * Send the query buffer.
		 */
		if (host_res_write(sock, &ns_sin, host, (char *) &query, (size_t) n) < 0) {
			(void) host_res_close(sock);
			set_h_errno(TRY_AGAIN);
			return (FALSE);
		}
	}

	do {
		/*
		 * Read the length of answer buffer.
		 */

		if (loading) {
			if ((len = cache_read_anslen()) < 0) {
				(void) cache_close(FALSE);
				set_h_errno(TRY_AGAIN);
				return (FALSE);
			}
		} else {
			if ((len = host_res_read_anslen(sock, &ns_sin, host)) < 0) {
				(void) host_res_close(sock);
				set_h_errno(TRY_AGAIN);
				return (FALSE);
			}
		}

		/*
		 * Terminate if length is zero.
		 */
		if (len == 0) {
			set_errno(EINVAL);
			host_res_perror(&ns_sin, host, "answer has length of zero");
			if (loading)
				(void) cache_close(FALSE);
			else
				(void) host_res_close(sock);
			set_h_errno(TRY_AGAIN);
			return (FALSE);
		}

		if (debug > 2)
			printf("%sexpecting an answer of %d bytes\n", debug_prefix, len);

		if (!(answer = (answer) ? realloc(answer, (size_t) len) : malloc((size_t) len))) {
			sys_error("unable to allocate %s byte buffer to hold %s for %s from %s",
				  dtoa(len), pr_type(T_AXFR), name, host);
			if (loading)
				(void) cache_close(FALSE);
			else
				(void) host_res_close(sock);
			set_h_errno(TRY_AGAIN);
			return (FALSE);
		}

		if (loading)
			n = cache_read(answer, (size_t) len);
		else
			n = host_res_read_stream(sock, &ns_sin, host, answer, (size_t) len);

		if (n < 0) {
			/* host_res_perror() already called */
			if (loading)
				(void) cache_close(FALSE);
			else
				(void) host_res_close(sock);
			set_h_errno(TRY_AGAIN);
			return (FALSE);
		}

		if (n == 0) {
			if (debug > 1)
				printf("%sgot EOF\n", debug_prefix);
			break;
		}

		set_errno(0);	/* reset after we have a possible answer */

		if (n < HFIXEDSZ) {
			pr_error("answer length of %s too short for a header during %s for %s from %s",
				 dtoa(n), pr_type(T_AXFR), name, host);
			if (loading)
				(void) cache_close(FALSE);
			else
				(void) host_res_close(sock);
			set_h_errno(TRY_AGAIN);
			return (FALSE);
		}

		if (n != len) {
			pr_error("answer length of %s too %s during %s for %s from %s",
				 dtoa(n), (n < len) ? "short" : "long", pr_type(T_AXFR), name, host);
			if (loading)
				(void) cache_close(FALSE);
			else
				(void) host_res_close(sock);
			set_h_errno(TRY_AGAIN);
			return (FALSE);
		}

		if (debug > 2)
			printf("%sgot an answer of %d bytes\n", debug_prefix, n);

		if (debug > 1)
			pr_query((qbuf_t *) answer, n, stdout);

		if (debug > 1)
			print_answer((querybuf_t *) answer, (size_t) n, T_AXFR);

		/*
		 * Analyze the contents of the answer and check for errors.  An
		 * error can be expected only in the very first packet.  The
		 * query section should be empty except in the first packet.
		 * Note the special error status codes for specific failures.
		 */
		bp = (HEADER *) answer;
		ancount = ntohs((u_short) bp->ancount);

		if (bp->rcode != NOERROR || ancount == 0) {
			switch (bp->rcode) {
			case NXDOMAIN:
				/* distinguish between authoritative or not */
				set_h_errno(bp->aa ? HOST_NOT_FOUND : NO_HOST);
				break;

			case NOERROR:
				/* distinguish between authoritative or not */
				set_h_errno(bp->aa ? NO_DATA : NO_RREC);
				break;

			case REFUSED:
				/* special status if zone transfer refused */
				set_h_errno(QUERY_REFUSED);
				break;

			case SERVFAIL:
				/* special status upon explicit failure */
				set_h_errno(SERVER_FAILURE);
				break;

			default:
				/* all other errors will cause a retry */
				set_h_errno(TRY_AGAIN);
				break;
			}
			if (npackets != 0) {
				pr_error("unexpected error during %s for %s from %s",
					 pr_type(T_AXFR), name, host);
			}
			if (loading)
				(void) cache_close(FALSE);
			else
				(void) host_res_close(sock);
			return (FALSE);
		}

		/* valid answer received, avoid buffer overrun */
		set_h_errno(0);

		/*
		 * The nameserver and additional info section should be empty.
		 * There may be multiple answers in the answer section.
		 */
#ifdef obsolete
		if (ancount > 1) {
			pr_error("multiple answers during %s for %s from %s",
				 pr_type(T_AXFR), name, host);
		}
#endif
		if (ntohs((u_short)bp->nscount) != 0) {
			pr_error("nonzero nscount during %s for %s from %s",
				 pr_type(T_AXFR), name, host);
		}
		if (ntohs((u_short)bp->arcount) != 0) {
			pr_error("nonzero arcount during %s for %s from %s",
				 pr_type(T_AXFR), name, host);
		}
		/*
		 * Valid packet received. Print contents if appropriate.
		 * Specific zone information will be saved by update_zone().
		 */
		npackets += 1;
		nrecords += ancount;

		soaname = NULL, subname = NULL, adrname = NULL, address = 0;
		listhost = host;

		(void) print_info((querybuf_t *) answer, n, name, T_AXFR, queryclass, FALSE);

#ifdef notyet
		/* make answer authoritative if it comes from such server */
		if (dumping && authserver)
			bp->aa = 1;
#endif
		/*
		 * Dump data to cache if so requested.
		 */
		if (dumping && (cache_write(answer, (size_t) n) < 0)) {
			(void) host_res_close(sock);
			set_h_errno(CACHE_ERROR);
			return (FALSE);
		}
	} while (n > 0 && soacount < 2);

	/*
	 * Note the case where we got an EOF reading before we see the second
	 * SOA denoting true "end of zone".
	 */
	if (soacount != 2) {
		pr_error("missing trailing SOA during %s for %s from %s",
			 pr_type(T_AXFR), name, host);
	}
	/*
	 * Write a zero length trailer to the cache to indicate end-of-file.
	 * This is not strictly necessary if the second SOA marks the end.
	 */
	if (dumping && (cache_write(answer, (size_t) 0) < 0)) {
		assert(!loading);
		(void) host_res_close(sock);
		set_h_errno(CACHE_ERROR);
		return (FALSE);
	}
	/*
	 * End of zone transfer at second SOA record or zero length read.
	 */
	if (loading)
		(void) cache_close(FALSE);
	else
		(void) host_res_close(sock);

	/*
	 * Check for the anomaly that the whole transfer consisted of the
	 * SOA records only. Could occur if we queried the victim of a lame
	 * delegation which happened to have the SOA record present.
	 */
	if (nrecords <= soacount) {
		pr_error("empty zone transfer for %s from %s",
			 name, host);
		set_h_errno(NO_RREC);
		return (FALSE);
	}

	/*
	 * Do an extra check for delegated zones that also have an A record.
	 * Those may have been defined in the child zone, and crept in the
	 * parent zone, or may have been defined as glue records.
	 *
	 * This is not necessarily an error, but the host count may be wrong.
	 * Note that an A record for the current zone has been ignored above.
	 * Skip this check if explicitly requested in quick mode, or in case
	 * nothing would be printed anyway in quiet mode.
	 */
	i = (quiet || quick) ? zonecount : 0;

	for (n = i; n < zonecount; n++) {
		i = host_index(zonename[n], FALSE);
#ifdef obsolete
		for (i = 0; i < hostcount; i++) {
			if (sameword(hostname(i), zonename[n]))
				break;	/* found */
		}
#endif
		if (i < hostcount) {
			pr_warning("%s has both NS and A records within %s from %s",
				   zonename[n], name, host);
		}
	}

	/*
	 * The zone transfer has been successful.
	 */
	if (verbose) {
		printf("Transfer complete, %d record%s received for %s\n",
		       nrecords, plural(nrecords), name);
		if (npackets != nrecords) {
			printf("Transfer consisted of %d packet%s from %s\n",
			       npackets, plural(npackets), host);
		}
	}

	return (TRUE);
}

/*
** UPDATE_ZONE -- Save zone information during zone listings
** ---------------------------------------------------------
**
**	Returns:
**		None.
**
**	Side effects:
**		Stores list of delegated zones found in zonename[],
**		and the count of delegated zones in ``zonecount''.
**		Stores list of host names found in hostname[],
**		and the count of host names in ``hostcount''.
**		Stores the count of SOA records in ``soacount''.
**
**	This routine is called by print_info() for each resource record.
*/

void
update_zone(name)
	input const char *name;		/* name of zone to do zone xfer for */
{
	char *host = listhost;		/* contacted host for zone listings */
	register int i;

	/*
	 * Terminate upon the second SOA record for this zone.
	 */
	if (soaname && sameword(soaname, name))
		soacount++;
	else if (soaname && !sameword(soaname, name)) {
		/* the nameserver balks on this one */
		pr_warning("extraneous SOA record for %s within %s from %s",
			   soaname, name, host);
	}
	/*
	 * Save encountered delegated zone name for recursive listing.
	 */
	if (subname && indomain(subname, name, FALSE)) {
		i = zone_index(subname, TRUE);
#ifdef obsolete
		for (i = 0; i < zonecount; i++) {
			if (sameword(zonename[i], subname))
				break;	/* duplicate */
		}
#endif
		if (i >= zonecount) {
			zonename = newlist(zonename, zonecount+1, char *);
			zonename[zonecount] = newstr(subname);
			zonecount++;
		}
	} else if (subname && !indomain(subname, name, TRUE)) {
		/* warn about strange delegated zones */
		pr_warning("extraneous NS record for %s within %s from %s",
			   subname, name, host);
	}

	/*
	 * Save encountered name of A record for host name count.
	 */
	if (adrname && indomain(adrname, name, FALSE) && !reverse) {
		i = host_index(adrname, TRUE);
#ifdef obsolete
		for (i = 0; i < hostcount; i++) {
			if (sameword(hostname(i), adrname))
				break;	/* duplicate */
		}
#endif
		if (i >= hostcount) {
			if (hostcount >= maxhosts) {
				maxhosts += MAXHOSTINCR;
				hostlist = newlist(hostlist, maxhosts, host_data_t);
			}
			hostname(hostcount) = newstr(adrname);
			hostaddr(hostcount) = address;
			multaddr(hostcount) = FALSE;
			hostcount++;
		} else if (address != hostaddr(i))
			multaddr(i) = TRUE;
	} else if (adrname && !indomain(adrname, name, TRUE)) {
		/* check for unauthoritative glue records */
		pr_warning("extraneous glue record for %s within %s from %s",
			   adrname, name, host);
	}

	return;
}

/*
** GET_MXREC -- Fetch MX records of a domain
** -----------------------------------------
**
**	Returns:
**		TRUE if MX records were found.
**		FALSE otherwise.
*/

bool_t
get_mxrec(name)
	input char *name;		/* domain name to get mx for */
{
	querybuf_t answer;
	register int n;

	if (verbose)
		printf("Finding MX records for %s ...\n", name);

	if ((n = get_info(&answer, name, T_MX, queryclass)) < 0)
		return (FALSE);

	(void) print_info(&answer, n, name, T_MX, queryclass, FALSE);

	return (TRUE);
}

/*
** GET_PRIMARY -- Fetch name of primary nameserver for a zone
** ----------------------------------------------------------
**
**	Returns:
**		Pointer to the name of the primary server, if found.
**		NULL if the server could not be determined.
*/

char *
get_primary(name)
	input char *name;		/* name of zone to get soa for */
{
	querybuf_t answer;
	register int n;

	if (verbose)
		printf("Finding primary nameserver for %s ...\n", name);

	if ((n = get_info(&answer, name, T_SOA, queryclass)) < 0)
		return (NULL);

	if (verbose > 1)
		(void) print_info(&answer, n, name, T_SOA, queryclass, FALSE);

	soaname = NULL;			/* XXX GAK! global side-effects! */
	(void) get_soainfo(&answer, n, name, T_SOA, queryclass);
	if (soaname == NULL)
		return (NULL);

	return (soa.primary);
}

/*
** CHECK_ZONE -- Fetch and analyze SOA record of a zone
** ----------------------------------------------------
**
**	Returns:
**		TRUE if the SOA record was found at the given server.
**		FALSE otherwise.
**
**	Inputs:
**		The global variable ``server'' must contain the name of
**		the server that was queried, and the resolver database
**		must have been reset with its addresses.
*/

bool_t
check_zone(name, host)
	input char *name;		/* name of zone to get soa for */
	input char *host;		/* name of server to be queried */
{
	querybuf_t answer;
	register int n;

	if (verbose)
		printf("Checking SOA for %s at %s ...\n", name, host); /* XXX show host's address too */
	else if (authserver)
		printf("%-20s\tNS\t%s\n", name, host);
	else
		printf("%-20s\t(%s)\n", name, host);

	if ((n = get_info(&answer, name, T_SOA, queryclass)) < 0)
		return (FALSE);

	if (verbose > 1)
		(void) print_info(&answer, n, name, T_SOA, queryclass, FALSE);

	soaname = NULL;			/* XXX GAK! global side-effects! */
	(void) get_soainfo(&answer, n, name, T_SOA, queryclass);
	if (soaname == NULL)
		return (FALSE);

	check_soa(&answer, name, host);

	return (TRUE);
}

/*
** CHECK_CACHE -- Fetch and analyze SOA record of a zone from the cache
** --------------------------------------------------------------------
**
**	Returns:
**		TRUE if the SOA record was found at the cache.
**		FALSE otherwise.
**
**	The very first record is retrieved from the cache.
**	Note that in the query section the type is AXFR, not SOA.
**	This implies that we cannot call print_info() here.
*/

static bool_t
check_cache(name, host)
	input char *name;		/* name of zone to get soa for */
	input char *host;		/* name of server to be queried */
{
	querybuf_t answer;
	register int n;

	if (verbose)
		printf("Checking SOA for %s at %s ...\n", name, host);
	else
		printf("%-20s\t(%s)\n", name, host);

	if ((n = load_soa(&answer, name)) < 0)
		return (FALSE);

	soaname = NULL;			/* XXX GAK! global side-effects! */
	(void) get_soainfo(&answer, n, name, T_AXFR, queryclass);
	if (soaname == NULL)
		return (FALSE);

	check_soa(&answer, name, host);

	return (TRUE);
}

/*
** COMPARE_SOA -- Compare SOA serial numbers in cache and reality
** --------------------------------------------------------------
**
**	Returns:
**		TRUE if both serial numbers exist, and are the same,
**		or in case a load from the cache is forced.
**		FALSE otherwise.
**
**	Note. The live serial number is retrieved via an ordinary
**	regular query, and not directly from any of the nameservers
**	because we have not looked up them yet. It may need refinement.
*/

bool_t
compare_soa(name)
	input char *name;		/* name of zone to get soa for */
{
	int serial1, serial2;
	querybuf_t answer;
	register int n;

	if (verbose)
		printf("Comparing SOA serial for %s ...\n", name);

	/*
	 * Fetch the serial number from the cache.
	 */
	if ((n = load_soa(&answer, name)) < 0)
		goto error1;

	soaname = NULL;			/* XXX GAK! global side-effects! */
	(void) get_soainfo(&answer, n, name, T_AXFR, queryclass);
	if (soaname == NULL)
		goto error1;

	serial1 = soa.serial;

	/*
	 * Force a load from the cache in case it is more recent than
	 * a certain reference time in the past, if specified.
	 */
	if (loadtime > 0 && cachetime > loadtime)
		return (TRUE);

	/*
	 * Fetch the live serial number.
	 */
	if ((n = get_info(&answer, name, T_SOA, queryclass)) < 0)
		goto error2;

	soaname = NULL;			/* XXX GAK! global side-effects! */
	(void) get_soainfo(&answer, n, name, T_SOA, queryclass);
	if (soaname == NULL)
		goto error2;

	serial2 = soa.serial;

	/*
	 * Report the result.
	 */
	return ((serial1 == serial2) ? TRUE : FALSE);

error1:
	/* no serial number from the cache */
	if (verbose)
		ns_error(name, T_SOA, queryclass, "cache");
	return (FALSE);

error2:
	/* no live serial number found */
	if (verbose)
		ns_error(name, T_SOA, queryclass, server);
	return (FALSE);
}

/*
** GET_SOAINFO -- Extract SOA data from nameserver answer buffer
** -------------------------------------------------------------
**
**	Returns:
**		TRUE if the answer buffer was processed successfully.
**		FALSE otherwise.
**
**	Outputs:
**		The global struct ``soa'' is filled with the soa data.
**
**	Side effects:
**		Sets ``soaname'' if there is a valid SOA record.
**		This variable must have been cleared before calling
**		get_soainfo() and may be checked afterwards.
*/

bool_t
get_soainfo(answerbuf, answerlen, name, qtype, qclass)
	input querybuf_t *answerbuf;	/* location of answer buffer */
	input int answerlen;		/* length of answer buffer */
	input char *name;		/* name of zone to get soa for */
	input int qtype;		/* record type we are querying about */
	input int qclass;		/* record class we are querying about */
{
	HEADER *bp;
	int qdcount, ancount;
	u_char *msg, *eom;
	register u_char *cp;

	bp = (HEADER *) answerbuf;
	qdcount = ntohs((u_short) bp->qdcount);
	ancount = ntohs((u_short) bp->ancount);

	msg = (u_char *) answerbuf;
	eom = (u_char *) answerbuf + answerlen;
	cp  = (u_char *) answerbuf + HFIXEDSZ;

	if (qdcount > 0 && cp < eom) {	/* should be exactly one record */
		if (!(cp = skip_qrec(name, qtype, qclass, cp, msg, eom)))
			return (FALSE);
		qdcount--;
	}
	if (qdcount) {
		pr_error("invalid qdcount after %s query for %s",
			 pr_type(qtype), name);
		set_h_errno(NO_RECOVERY);
		return (FALSE);
	}

	/*
	 * Check answer section only.
	 * Check that answers match the requested zone. Ignore other entries.
	 * The nameserver section may contain the nameservers for the zone,
	 * and the additional section their addresses, but not guaranteed.
	 * Those sections are usually empty for authoritative answers.
	 */
	while (ancount > 0 && cp < eom) {
		char rname[MAXDNAME+1];
		int type, class, ttl, dlen;
		u_char *eor;
		register int n;

		if ((n = expand_name(name, T_NONE, cp, msg, eom, rname)) < 0)
			return (FALSE);
		cp += n;

		n = (3 * INT16SZ) + INT32SZ;
		if (check_size(rname, T_NONE, cp, msg, eom, n) < 0)
			return (FALSE);

		type = ns_get16(cp);
		cp += INT16SZ;

		class = ns_get16(cp);
		cp += INT16SZ;

		ttl = ns_get32(cp);
		cp += INT32SZ;

		dlen = ns_get16(cp);
		cp += INT16SZ;

		if (check_size(rname, type, cp, msg, eom, dlen) < 0)
			return (FALSE);
		eor = cp + dlen;
#ifdef DEBUG
		if (verbose > 2) {
			printf("get_soainfo(): DEBUG: %-20s\t%d\t%s\t%s\n",
			       rname, ttl, pr_class(class), pr_type(type));
		}
#endif
		if ((type == T_SOA) && sameword(rname, name)) {
			if ((n = expand_name(rname, type, cp, msg, eom, soa.primary)) < 0)
				return (FALSE);
			cp += n;

			if ((n = expand_name(rname, type, cp, msg, eom, soa.hostmaster)) < 0)
				return (FALSE);
			cp += n;

			n = 5 * INT32SZ;
			if (check_size(rname, type, cp, msg, eor, n) < 0)
				return (FALSE);
			soa.serial = ns_get32(cp);
			cp += INT32SZ;
			soa.refresh = ns_get32(cp);
			cp += INT32SZ;
			soa.retry = ns_get32(cp);
			cp += INT32SZ;
			soa.expire = ns_get32(cp);
			cp += INT32SZ;
			soa.nxdomttl = ns_get32(cp);
			cp += INT32SZ;

			/* valid complete soa record found */
			soaname = strcpy(soanamebuf, rname);
		} else {
			/* just ignore other records */
			cp += dlen;
		}
		if (cp != eor) {
			pr_error("size error in %s record for %s, off by %s",
				 pr_type(type), rname, dtoa((int) (cp - eor)));
			set_h_errno(NO_RECOVERY);
			return (FALSE);
		}
		ancount--;
	}
	if (ancount) {
		pr_error("invalid ancount after %s query for %s",
			 pr_type(qtype), name);
		set_h_errno(NO_RECOVERY);
		return (FALSE);
	}

	/* set proper status if no answers found */
	set_h_errno((soaname != NULL) ? 0 : TRY_AGAIN);

	return (TRUE);
}

/*
** LOAD_SOA -- Load the SOA record of a zone from the cache
** --------------------------------------------------------
**
**	Returns:
**		Length of answer buffer, if obtained.
**		-1 if no answer (h_errno is set appropriately).
**
**	This just reads the very first record from the cache.
*/

int
load_soa(answerbuf, name)
	output querybuf_t *answerbuf;	/* location of buffer to store answer */
	input char *name;		/* name of zone to check soa for */
{
	register int n;

	if (cache_open(name, FALSE) < 0) {
		set_h_errno(NO_RREC);
		return (-1);
	}

	if ((n = cache_read((char *) answerbuf, sizeof(querybuf_t))) < 0) {
		(void) cache_close(FALSE);
		set_h_errno(TRY_AGAIN);
		return (-1);
	}

	(void) cache_close(FALSE);
	set_h_errno(0);

	return (n);
}

/*
** CHECK_SOA -- Analyze retrieved SOA records of a zone
** ----------------------------------------------------
**
**	Returns:
**		None.
**
**	Inputs:
**		The global variable ``server'' must contain the name of
**		the server that was queried, and the resolver database
**		must have been reset with its addresses.
**		The global struct ``soa'' must contain the soa data.
*/

void
check_soa(answerbuf, name, host)
	input querybuf_t *answerbuf;	/* location of answer buffer */
	input char *name;		/* name of zone to check soa for */
	input char *host;		/* name of server to be queried */
{
	static char oldnamebuf[MAXDNAME+1];
	static char *oldname = NULL;	/* previous name of zone */
	static char *oldhost = NULL;	/* previous name of server */
	static soa_data_t oldsoa;	/* previous soa data */
	register int n;
	HEADER *bp;

	/*
	 * Print the various SOA fields in abbreviated form.
	 * Values are actually unsigned, but we print them as signed integers,
	 * apart from the serial which really becomes that big sometimes.
	 * In the latter case we print a warning below.
	 */
	printf("%s\t%s\t(%u %d %d %d %d)\n",
	       soa.primary, soa.hostmaster, (unsigned) soa.serial,
	       soa.refresh, soa.retry, soa.expire, soa.nxdomttl);

	/*
	 * We are supposed to have queried an authoritative nameserver, and since
	 * nameserver recursion has been turned off, answer must be authoritative.
	 * An answer retrieved from the local cache is never marked authoritative.
	 */
	bp = (HEADER *) answerbuf;
	if (!bp->aa && !sameword(host, "cache")) {
		if (authserver) {
			pr_error("%s SOA record at %s is not authoritative",
				 name, host);
		} else {
			pr_warning("%s SOA record at %s is not authoritative",
				   name, host);
		}
		if (authserver) {
			pr_error("no SOA: %s has lame delegation to %s",
				 name, host);
		}
	}

	/*
	 * Check whether we are switching to a new zone.
	 * The old name must have been saved in static storage.
	 */
	if ((oldname != NULL) && !sameword(name, oldname))
		oldname = NULL;

	/*
	 * Make few timer consistency checks only for the first one in a series.
	 * Compare the primary field against the list of authoritative servers.
	 * Explicitly check the hostmaster field for invalid characters ('@').
	 * Yell if the serial has the high bit set (not always intentional).
	 * Make sanity checks for refresh and retry times.
	 * Check for bizarre expire values.
	 */
	if (oldname == NULL) {
		for (n = 0; n < nservers; n++) {
			if (sameword(soa.primary, nsname[n]))
				break;	/* found */
		}
		if ((n >= nservers) && authserver) {
			pr_warning("%s SOA primary %s is not advertised via NS",
				   name, soa.primary);
		}
		if (!valid_name(soa.primary, FALSE, FALSE, FALSE)) {
			pr_warning("%s SOA primary %s has an invalid name",
				   name, soa.primary);
		}
		if (!valid_name(soa.hostmaster, FALSE, TRUE, FALSE)) {
			pr_warning("%s SOA hostmaster %s has an invalid mailbox specification",
				   name, soa.hostmaster);
		}
		if (bitset(0x80000000U, soa.serial)) {
			pr_warning("%s SOA serial has high bit set",
				   name);
		}
		if (soa.retry > soa.refresh) {
			pr_warning("%s SOA retry exceeds refresh",
				   name);
		}
		if (soa.refresh + soa.retry > soa.expire) {
			pr_warning("%s SOA refresh+retry exceeds expire",
				   name);
		}
		if (soa.expire < 1 * 7 * 24 * 3600) {
			pr_warning("%s SOA expire is less than 1 week (%s)",
				   name, pr_time(soa.expire, FALSE));
		}
		if (soa.expire > 26 * 7 * 24 * 3600) {
			pr_warning("%s SOA expire is more than 6 months (%s)",
				   name, pr_time(soa.expire, FALSE));
		}
	}

	/*
	 * Compare various fields with those of the previous query, if any.
	 * Different serial numbers may be present if secondaries have not yet
	 * refreshed the data from the primary. Issue only a warning in that case.
	 */
	if (oldname != NULL) {
		if (!sameword(soa.primary, oldsoa.primary)) {
			pr_error("%s and %s have different primary for %s",
				 host, oldhost, name);
		}
		if (!sameword(soa.hostmaster, oldsoa.hostmaster)) {
			pr_error("%s and %s have different hostmaster for %s",
				 host, oldhost, name);
		}
		if (soa.serial != oldsoa.serial) {
			pr_warning("%s and %s have different serial for %s",
				   host, oldhost, name);
		}
		if (soa.refresh != oldsoa.refresh) {
			pr_error("%s and %s have different refresh for %s",
				 host, oldhost, name);
		}
		if (soa.retry != oldsoa.retry) {
			pr_error("%s and %s have different retry for %s",
				 host, oldhost, name);
		}
		if (soa.expire != oldsoa.expire) {
			pr_error("%s and %s have different expire for %s",
				 host, oldhost, name);
		}
		if (soa.nxdomttl != oldsoa.nxdomttl) {
			pr_error("%s and %s have different nxdomttl for %s",
				 host, oldhost, name);
		}
	}
	
	/*
	 * Save the current information.
	 */
	oldname = strcpy(oldnamebuf, name);
	oldhost = host;
	oldsoa = soa;

	return;
}

/*
** CHECK_DUPL -- Check global address list for duplicates
** ------------------------------------------------------
**
**	Returns:
**		TRUE if the given host address already exists.
**		FALSE otherwise.
**
**	Side effects:
**		Adds the host address to the list if not present.
**
**	The information in this table is global, and is not cleared.
*/

#define AHASHSIZE	0x2000
#define AHASHMASK	0x1fff

typedef struct addr_tab {
	ipaddr_t *addrlist;		/* global list of addresses */
	int addrcount;			/* count of global addresses */
} addr_tab_t;

addr_tab_t addrtab[AHASHSIZE];		/* hash list of global addresses */

bool_t
check_dupl(addr)
	input ipaddr_t addr;		/* address of host to check */
{
 	register int i;
	register addr_tab_t *s;

	s = &addrtab[ntohl(addr) & AHASHMASK];

	for (i = 0; i < s->addrcount; i++) {
		if (s->addrlist[i] == addr)
			return (TRUE);	/* duplicate */
	}
	s->addrlist = newlist(s->addrlist, s->addrcount + 1, ipaddr_t);
	s->addrlist[s->addrcount] = addr;
	s->addrcount++;

	return (FALSE);
}

/*
** CHECK_TTL -- Check list of records for different TTL values
** -----------------------------------------------------------
**
**	Returns:
**		TRUE if the TTL value matches the first record
**		already listed with the same name/type/class.
**		FALSE only when the first discrepancy is found.
**
**	Side effects:
**		Adds the record data to the list if not present.
*/

typedef struct ttl_tab {
	struct ttl_tab *next;		/* next entry in chain */
	char *name;			/* name of resource record */
	int type;			/* resource record type */
	int class;			/* resource record class */
	int ttl;			/* time_to_live value */
	int count;			/* count of different TTL values */
} ttl_tab_t;

ttl_tab_t *ttltab[HASHSIZE];		/* hash list of record info */

bool_t
check_ttl(name, type, class, ttl)
	input char *name;		/* resource record name */
	input int type, class, ttl;	/* resource record fixed values */
{
	register ttl_tab_t *s;
	register ttl_tab_t **ps;
	register unsigned int hfunc;
	register char *p;
	register char c;

	/*
	 * Compute the hash function for this resource record.
	 * Look it up in the appropriate hash chain.
	 */
	for (hfunc = type, p = name; (c = *p) != '\0'; p++) {
		hfunc = ((hfunc << 1) ^ (lowercase((int) c) & 0377)) % HASHSIZE;
	}
	for (ps = &ttltab[hfunc]; (s = *ps) != NULL; ps = &s->next) {
		if (s->type != type || s->class != class)
			continue;
		if (sameword(s->name, name))
			break;
	}

	/*
	 * Allocate new entry if not found.
	 */
	if (s == NULL) {
		/* ps = &ttltab[hfunc]; */
		s = newstruct(ttl_tab_t);

		/* initialize new entry */
		s->name = newstr(name);
		s->type = type;
		s->class = class;
		s->ttl = ttl;
		s->count = 0;

		/* link it in */
		s->next = *ps;
		*ps = s;
	}

	/*
	 * Check whether the TTL value matches the first recorded one.
	 * If not, signal only the first discrepancy encountered, so
	 * only one warning message will be printed.
	 */
	if (s->ttl == ttl)
		return (TRUE);

	s->count += 1;

	return ((s->count == 1) ? FALSE : TRUE);
}

/*
** CLEAR_TTLTAB -- Clear resource record list for TTL checking
** -----------------------------------------------------------
**
**	Returns:
**		None.
**
**	An entry on the hash list, and the host name in each
**	entry, have been allocated in dynamic memory.
**
**	The information in this table is on a per-zone basis.
**	It must be cleared before any subsequent zone transfers.
*/

void
clear_ttltab()
{
	register int i;
	register ttl_tab_t *s, *t;

	for (i = 0; i < HASHSIZE; i++) {
		if (ttltab[i] != NULL) {
			/* free chain of entries */
			for (t = NULL, s = ttltab[i]; s != NULL; s = t) {
				t = s->next;
				xfree(s->name);
				xfree(s);
			}

			/* reset hash chain */
			ttltab[i] = NULL;
		}
	}

	return;
}

/*
** HOST_INDEX -- Check list of host names for name being present
** -------------------------------------------------------------
**
**	Returns:
**		Index into hostname[] table, if found.
**		Current ``hostcount'' value, if not found.
**
**	Side effects:
**		May add an entry to the hash list if not present.
**
**	A linear search through the master table becomes very
**	costly for zones with more than a few thousand hosts.
**	Maintain a hash list with indexes into the master table.
**	Caller should update the master table after this call.
*/

typedef struct host_tab {
	struct host_tab *next;		/* next entry in chain */
	int slot;			/* slot in host name table */
} host_tab_t;

host_tab_t *hosttab[HASHSIZE];		/* hash list of host name info */

int
host_index(name, enter)
	input char *name;		/* the host name to check */
	input bool_t enter;		/* add to table if not found */
{
	register host_tab_t *s;
	register host_tab_t **ps;
	register unsigned int hfunc;
	register char *p;
	register char c;

	/*
	 * Compute the hash function for this host name.
	 * Look it up in the appropriate hash chain.
	 */
	for (hfunc = 0, p = name; (c = *p) != '\0'; p++) {
		hfunc = ((hfunc << 1) ^ (lowercase((int) c) & 0377)) % HASHSIZE;
	}
	for (ps = &hosttab[hfunc]; (s = *ps) != NULL; ps = &s->next) {
		if (s->slot >= hostcount)
			continue;
		if (sameword(hostname(s->slot), name))
			break;
	}

	/*
	 * Allocate new entry if not found.
	 */
	if ((s == NULL) && enter) {
		/* ps = &hosttab[hfunc]; */
		s = newstruct(host_tab_t);

		/* initialize new entry */
		s->slot = hostcount;

		/* link it in */
		s->next = *ps;
		*ps = s;
	}

	return ((s != NULL) ? s->slot : hostcount);
}

/*
** CLEAR_HOSTTAB -- Clear hash list for host name checking
** -------------------------------------------------------
**
**	Returns:
**		None.
**
**	A hash list entry has been allocated in dynamic memory.
**
**	The information in this table is on a per-zone basis.
**	It must be cleared before any subsequent zone transfers.
*/

void
clear_hosttab()
{
	register int i;
	register host_tab_t *s, *t;

	for (i = 0; i < HASHSIZE; i++) {
		if (hosttab[i] != NULL) {
			/* free chain of entries */
			for (t = NULL, s = hosttab[i]; s != NULL; s = t) {
				t = s->next;
				xfree(s);
			}
			/* reset hash chain */
			hosttab[i] = NULL;
		}
	}

	return;
}

/*
** ZONE_INDEX -- Check list of zone names for name being present
** -------------------------------------------------------------
**
**	Returns:
**		Index into zonename[] table, if found.
**		Current ``zonecount'' value, if not found.
**
**	Side effects:
**		May add an entry to the hash list if not present.
**
**	A linear search through the master table becomes very
**	costly for more than a few thousand delegated zones.
**	Maintain a hash list with indexes into the master table.
**	Caller should update the master table after this call.
*/

typedef struct zone_tab {
	struct zone_tab *next;		/* next entry in chain */
	int slot;			/* slot in zone name table */
} zone_tab_t;

zone_tab_t *zonetab[HASHSIZE];		/* hash list of zone name info */

int
zone_index(name, enter)
	input char *name;		/* the zone name to check */
	input bool_t enter;		/* add to table if not found */
{
	register zone_tab_t *s;
	register zone_tab_t **ps;
	register unsigned int hfunc;
	register char *p;
	register char c;

	/*
	 * Compute the hash function for this zone name.
	 * Look it up in the appropriate hash chain.
	 */
	for (hfunc = 0, p = name; (c = *p) != '\0'; p++) {
		hfunc = ((hfunc << 1) ^ (lowercase((int) c) & 0377)) % HASHSIZE;
	}
	for (ps = &zonetab[hfunc]; (s = *ps) != NULL; ps = &s->next) {
		if (s->slot >= zonecount)
			continue;
		if (sameword(zonename[s->slot], name))
			break;
	}

	/*
	 * Allocate new entry if not found.
	 */
	if ((s == NULL) && enter) {
		/* ps = &zonetab[hfunc]; */
		s = newstruct(zone_tab_t);

		/* initialize new entry */
		s->slot = zonecount;

		/* link it in */
		s->next = *ps;
		*ps = s;
	}

	return ((s != NULL) ? s->slot : zonecount);
}

/*
** CLEAR_ZONETAB -- Clear hash list for zone name checking
** -------------------------------------------------------
**
**	Returns:
**		None.
**
**	A hash list entry has been allocated in dynamic memory.
**
**	The information in this table is on a per-zone basis.
**	It must be cleared before any subsequent zone transfers.
*/

void
clear_zonetab()
{
	register int i;
	register zone_tab_t *s, *t;

	for (i = 0; i < HASHSIZE; i++) {
		if (zonetab[i] != NULL) {
			/* free chain of entries */
			for (t = NULL, s = zonetab[i]; s != NULL; s = t) {
				t = s->next;
				xfree(s);
			}

			/* reset hash chain */
			zonetab[i] = NULL;
		}
	}

	return;
}

/*
** CHECK_CANON -- Check list of domain names for name being canonical
** ------------------------------------------------------------------
**
**	Returns:
**		Nonzero if the name is definitely not canonical.
**		0 if it is canonical, or if it remains undecided.
**
**	Side effects:
**		Adds the domain name to the list if not present.
**
**	The information in this table is global, and is not cleared
**	(which may be necessary if the checking algorithm changes).
*/

typedef struct canon_tab {
	struct canon_tab *next;		/* next entry in chain */
	char *name;			/* domain name */
	int status;			/* nonzero if not canonical */
} canon_tab_t;

canon_tab_t *canontab[HASHSIZE];	/* hash list of domain name info */

int
check_canon(name)
	input char *name;		/* the domain name to check */
{
	register canon_tab_t *s;
	register canon_tab_t **ps;
	register unsigned int hfunc;
	register char *p;
	register char c;

	/*
	 * Compute the hash function for this domain name.
	 * Look it up in the appropriate hash chain.
	 */
	for (hfunc = 0, p = name; (c = *p) != '\0'; p++){
		hfunc = ((hfunc << 1) ^ (lowercase((int) c) & 0377)) % HASHSIZE;
	}

	for (ps = &canontab[hfunc]; (s = *ps) != NULL; ps = &s->next) {
		if (sameword(s->name, name))
			break;
	}

	/*
	 * Allocate new entry if not found.
	 * Only then is the actual check carried out.
	 */
	if (s == NULL) {
		/* ps = &canontab[hfunc]; */
		s = newstruct(canon_tab_t);

		/* initialize new entry */
		s->name = newstr(name);
		s->status = canonical(name);

		/* link it in */
		s->next = *ps;
		*ps = s;
	}

	return (s->status);
}
