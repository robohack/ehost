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

/*
 * Originally, this program came from Rutgers University, however it
 * is based on nslookup and other pieces of named tools, so it needs
 * the copyright notice above.
 */

/*
 * Originally rewritten by Eric Wassenaar, Nikhef-H, <e07@nikhef.nl>
 * Originally availalbe for FTP from the machine 'ftp.nikhef.nl'
 * in the directory '/pub/network'.
 *
 * Currently maintained by Greg A. Woods, Planix, Inc.; <woods-host@planix.com>
 *
 * The officially maintained source of this program is now available via
 * anonymous FTP from machine 'ftp.weird.com' in the directory '/pub/local' as
 * 'host.tar.Z'  <URL:ftp://ftp.weird.com/pub/local/host.tar.Z>
 *
 * You are kindly requested to report bugs and make suggestions for
 * improvements to the author at the given email address, and to not
 * re-distribute your own modifications to others.
 */

#ident "@(#)host:$Name:  $:$Id: main.c,v 1.29 2013-09-27 23:53:38 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)main.c	e07@nikhef.nl (Eric Wassenaar) 991529";
#endif

#include "host.h"
#define _DEFINE
#include "glob.h"

/*
 *			New features
 *
 * - Major overhaul of the entire code.
 * - Very rigid error checking, with more verbose error messages.
 * - Zone listing section completely rewritten.
 * - It is now possible to do recursive listings into delegated sub-zones.
 * - Maintain resource record statistics during zone listings.
 * - Maintain count of hosts during zone listings.
 * - Check for various extraneous conditions during zone listings.
 * - Check for invalid domain names containing invalid characters.
 * - Verify that certain domain names represent canonical host names.
 * - Perform TTL consistency checking during zone listings.
 * - Exploit multiple server addresses if available.
 * - Option to exploit only primary server for zone transfers.
 * - Option to exclude info from names that do not reside in a zone.
 * - Implement timeout handling during connect and read.
 * - Write resource record output to optional log file.
 * - Special MB tracing by recursively expanding MR and MG records.
 * - Special mode to check SOA records at each nameserver for a zone.
 * - Special mode to check reverse mappings of host addresses.
 * - Extended syntax allows multiple arguments on command line or stdin.
 * - Configurable default options in HOST_DEFAULTS environment variable.
 * - Implement new resource record types from RFC 1183 and 1348.
 * - Basic experimental NSAP support as defined in RFC 1637.
 * - Implement new resource record types from RFC 1664 and 1712.
 * - Implement new resource record types from RFC 1876 and 1886.
 * - Code is extensively documented.
 */

/*
 *			Publication history
 *
 * This information has been moved to the RELEASE_NOTES file.
 */

/*
 *			Compilation options
 *
 * This program usually compiles without special compilation options,
 * but for some platforms you may have to define special settings.
 * See the Makefile and the header file port.h for details.
 */

/*
 *			Miscellaneous notes
 *
 * This program should be linked explicitly with the BIND resolver library
 * in case the default gethostbyname() or gethostbyaddr() routines use a
 * non-standard strategy for retrieving information. These functions in the
 * resolver library call on the nameserver, and fall back on the hosts file
 * only if no nameserver is running (ECONNREFUSED).
 *
 * You may also want to link this program with the BIND resolver library if
 * your default library has not been compiled with DEBUG printout enabled.
 *
 * The version of the resolver should be BIND 4.8.2 or later. The crucial
 * include files are <netdb.h>, (resolv.h>, <arpa/nameser.h>. These files
 * are assumed to be present in the /usr/include directory.
 *
 * The resolver code depends on the definition of the BSD pre-processor
 * variable. This variable is usually defined in the file <sys/param.h>.
 *
 * The definition of this variable determines the method how to handle
 * datagram connections. This may not work properly on all platforms.
 *
 * The hostent struct defined in <netdb.h> is assumed to handle multiple
 * addresses in h_addr_list[]. Usually this is true if BSD >= 43.
 *
 * Your nameserver may not handle queries about top-level zones properly
 * if the "domain" directive is present in the named.boot file. It will
 * append the default domain to single names for which no data is cached.
 *
 * The treatment of TXT records has changed from 4.8.2 to 4.8.3. Formerly,
 * the data consisted simply of the text string. Now, the text string is
 * preceded by the character count with a maximum of 255, and multiple
 * strings are embedded if the total character count exceeds 255.
 * We handle only the new situation in this program, assuming that nobody
 * uses TXT records before 4.8.3 (unfortunately this is not always true:
 * current vendor supplied software may sometimes be even pre-BIND 4.8.2).
 *
 * Note that in 4.8.3 PACKETSZ from nameser.h is still at 512, which is
 * the maximum possible packet size for datagrams, whereas MAXDATA from
 * db.h has increased from 256 to 2048. The resolver defines MAXPACKET
 * as 1024. The nameserver reads queries in a buffer of size BUFSIZ.
 *
 * The gethostbyname() routine in 4.8.3 interprets dotted quads (if not
 * terminated with a dot) and simulates a gethostbyaddr(), but we will
 * not rely on it, and handle dotted quads ourselves.
 *
 * On some systems a bug in the _doprnt() routine exists which prevents
 * printf("%.*s", n, string) to be printed correctly if n == 0.
 *
 * This program has not been optimized for speed. Especially the memory
 * management is simple and straightforward.
 */

/*
 *			Terminology used
 *
 * Gateway hosts.
 * These are hosts that have more than one address registered under
 * the same name. Obviously we cannot recognize a gateway host if it
 * has different names associated with its different addresses.
 *
 * Duplicate hosts.
 * These are non-gateway hosts of which the address was found earlier
 * but with a different name, possibly in a totally different zone.
 * Such hosts should not be counted again in the overall host count.
 * This situation notably occurs in e.g. the "ac.uk" domain which has
 * many names registered in both the long and the abbreviated form,
 * such as 'host.department.university.ac.uk' and 'host.dept.un.ac.uk'.
 * This is probably not an error per se. It is an error if some domain
 * has registered a foreign address under a name within its own domain.
 * To recognize duplicate hosts when traversing many zones, we have to
 * maintain a global list of host addresses. To simplify things, only
 * single-address hosts are handled as such.
 *
 * Extrazone hosts.
 * These are hosts which belong to a zone but which are not residing
 * directly within the zone under consideration and which are not
 * glue records for a delegated zone of the given zone. E.g. if we are
 * processing the zone 'bar' and find 'host.foo.bar' but 'foo.bar' is not
 * an NS registered delegated zone of 'bar' then it is considered to be
 * an extrazone host. This is not necessarily an error, but it could be.
 *
 * Lame delegations.
 * If we query the SOA record of a zone at a supposedly authoritative
 * nameserver for that zone (listed in the NS records for the zone),
 * the SOA record should be present and the answer authoritative.
 * If not, we flag a lame delegation of the zone to that nameserver.
 * This may need refinement in some special cases.
 * A lame delegation is also flagged if we discover that a nameserver
 * mentioned in an NS record does not exist when looking up its address.
 *
 * Primary nameserver.
 * This utility assumes that the first domain name in the first field of the
 * RHS of the SOA record for a zone contains the name of the primary nameserver
 * (or one of the primary nameservers) for that zone (as defined by RFC 1033
 * and 1035, and clarified in RFC 2181.
 */

/*
 *		Usage: host [options] name [server]
 *		Usage: host [options] -x [name ...]
 *		Usage: host [options] -X server [name ...]
 *
 * Special options.
 * ---------------
 *
 * -j minport	first source address port in explicit range
 * -J maxport	last  source address port in explicit range
 * -O srcaddr	explicit source address IP number
 *
 * Undocumented options. (Experimental, subject to change)
 * --------------------
 *
 * -B		enforce full BIND behavior during DNSRCH
 * -g length	only select names that are at least this long
 * -K		print timestamp messages during zone listings
 * -M		special mode to list mailable delegated zones of zone
 * -W		special mode to list wildcard records in a zone
 * -Y		dump data of resource record after regular printout
 * -z		special mode to list delegated zones in a zone
 *
 * Available unuxed option letters.
 * -------------------------------
 *
 * -b
 * -k
 * -U
 * -y
 */

static char usage_msg[] =
"\
Usage:      host [-v] [-a] [-t querytype] [options]  name  [server]\n\
Long Help:  host -h|--help\n\
Listing:    host [-v] [-a] [-t querytype] [options]  -l zone  [server]\n\
Hostcount:  host [-v] [options] -H [-D] [-E] [-G] zone\n\
Check SOA:  host [-v] [options] -C zone\n\
Addrcheck:  host [-v] [options] -A host\n\
Extended usage:  host [options] -x name ...\n\
                 host [options] -X server name ...\n\
Listing options: [-L level] [-S] [-A] [-N skipzone]\n\
Common options:  [-d] [-f|-F file] [-I chars] [-i|-n] [-p] [-q] [-Q] [-T] [-Z]\n\
Other options:   [-c class] [-e] [-m] [-o] [-P] [-r] [-R] [-s secs] [-u] [-w]\n\
Special options: [-O srcaddr] [-j minport] [-J maxport]\n\
";

static char Help[] =
"\
-A	  --addrcheck		check reverse address mapping\n\
-a	  --anything		filter any resource record available\n\
	  --canoncheck		always check for canonical host names\n\
	  --canonskip		do not check for canonical host names\n\
-C	  --checksoa		check soa record at each of the nameservers\n\
-CAL 1	  --checkzone		perform extensive analysis of zone data\n\
-c class  --class=queryclass	specify explicit query class\n\
	  --cnamecheck		check cname target for existence\n\
	  --compare[=period]	compare soa serial before zone transfer\n\
-d	  --debug		enable debug printout (the more the merrier)\n\
	  --dump[=cachedir]	dump zone data to local disk cache\n\
-e	  --exclusive		don't print resource records outside domain\n\
-x	  --extended		extended usage with multiple arguments\n\
-f	  --file=filename	log resource record output also in file\n\
-Z	  --full		resource record output in full zone format\n\
-h	  --help		show summary of long command line options\n\
-H	  --hostcount		generate hostcount statistics\n\
-i	  --inaddr		construct reverse in-addr.arpa query\n\
-J maxport			specify a maximum port # for the source port\n\
-j minport			specify a minimum port # for the source port\n\
-L level  --depth=level		specify max recursion level during zone listing\n\
-l	  --list		generate zone listing\n\
	  --load[=cachedir]	load zone data from local disk cache\n\
-m				specify query type T_MAILB and trace MB records\n\
-O srcaddr			specify a source address for the query\n\
-o	  --suppress		don't print resource records to stdout\n\
-r	  --norecurs		turn off recursion in nameserver queries\n\
	  --nothing		no resource record output during zone listing\n\
-P	  --parent		use the nameservers found in the parent zone\n\
-p	  --primary		get answers from soa primary nameserver only\n\
	  --prefserver=list	set the preferred nameservers (comma sep list)\n\
-Q	  --quick		skip time consuming special checks\n\
-q	  --quiet		suppress all non-fatal warning messages\n\
	  --recursive		zone listing with near-infinite recursion level\n\
	  --retry=count		specify retry count for datagram queries\n\
	  --server=host		specify explicit nameserver to query\n\
-R				enable RES_DNSRCH (resolver 'search' option)\n\
-r	  --norecurs		do not use recursion when querying nameserver\n\
-S	  --statistics		generate resource record statistics\n\
-s secs	  --timeout=seconds	specify timeout for nameserver queries\n\
-t type	  --type=querytype	specify explicit query type\n\
	  --usage		show summary of short command line syntax\n\
-u	  --tcp			use tcp instead of udp for nameserver queries\n\
-v	  --verbose		enable verbose printout (repeat to incr level)\n\
-V	  --version		show program version number\n\
-w				wait until nameserver becomes available\n\
-X srvr	  --server=srvr		specify the name of the server to query\n\
-_	  --undercheck		check for invalid underscores in hostsnames\n\
";

/*
** MAIN -- Start of program host
** -----------------------------
**
**	Exits:
**		EX_OK		Operation successfully completed
**		EX_UNAVAILABLE	Could not obtain requested information
**		EX_CANTCREAT	Could not create specified log file
**		EX_NOINPUT	No input arguments were found
**		EX_NOHOST	Could not lookup explicit server
**		EX_OSERR	Could not obtain resources
**		EX_USAGE	Improper parameter/option specified
**		EX_SOFTWARE	Assertion botch in DEBUG mode
*/

static res_state_t new_res;	/* new resolver database */

static char **optargv = NULL;	/* argument list including default options */
static int optargc = 0;		/* number of arguments in new argument list */

static char *servername = NULL;		/* name of explicit server to query */
static char *logfilename = NULL;	/* name of log file to store output */
static char *cachedirname = NULL;	/* name of local cache directory */

char *argv0 = "HOST";		/* name that host was called with */

static void usage_error __P((const char *, ...));

int
main(argc, argv)
	input int argc;
	input char *argv[];
{
	register char *option;
	int result;			/* result status of action taken */
	bool_t extended = FALSE;	/* accept extended argument syntax */

	assert(sizeof(int) >= 4);	/* probably paranoid */
	assert(INADDR_NONE == NOT_DOTTED_QUAD); /* Houston, we have a problem! */
#ifdef obsolete
	assert(sizeof(u_short) == 2);	/* perhaps less paranoid */
	assert(sizeof(ipaddr_t) == 4);	/* but this is critical */
#endif

	/*
	 * Synchronize stdout and stderr in case output is redirected.
	 */
	linebufmode(stdout);

	/*
	 * Avoid premature abort when remote peer closes the connection.
	 */
	setsignal(SIGPIPE, SIG_IGN);

	/*
	 * Initialize resolver, set new defaults. See show_res() for details.
	 * The old defaults are (RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)
	 */
	(void) res_init();

	_res.options |=  RES_DEFNAMES;	/* qualify single names */
	_res.options &= ~((unsigned) RES_DNSRCH);	/* dotted names are qualified */

	_res.options |=  RES_RECURSE;	/* request nameserver recursion */
	_res.options &= ~((unsigned) RES_DEBUG);	/* turn off debug printout */
	_res.options &= ~((unsigned) RES_USEVC);	/* do not use virtual circuit */

	_res.retry = DEF_RETRIES;	/* number of datagram retries */
	_res.retrans = DEF_RETRANS;	/* timeout in secs between retries */

	/* initialize packet id */
	_res.id = getpid() & 0x7fff;

	/* save new defaults */
	new_res = _res;

	/*
	 * Check whether host was called with a different name.
	 * Interpolate default options and parameters.
	 */
	if (argc <= 1 || argv[0] == NULL)
		usage_error(usage_msg);

	if ((option = getenv("HOST_DEFAULTS"))) {
		set_defaults(option, argc, argv);
		argc = optargc; argv = optargv;
	}

	argv0 = (argv0 = strrchr(argv[0], '/')) ? argv0 + 1 : argv[0];

	/* check for resource record names */
	if ((querytype = parse_type(argv0)) < 0)
		querytype = T_NONE;

	/* set default class */
	queryclass = C_IN;

	/* check for zone listing abbreviation */
	if (sameword(argv0, "zone"))
		listmode = TRUE;

	/*
	 * Scan command line options and flags.
	 */
	while (argc > 1 && argv[1] != NULL && argv[1][0] == '-') {
		for (option = &argv[1][1]; *option != '\0'; option++) {
			switch (*option) {
			case '-' :
				if (option == &argv[1][1])
					option = cvtopt(option+1);
				break;

			case 'A' :
				addrmode = TRUE;
				break;

			case 'a' :
				querytype = T_ANY;	/* filter anything available */
				break;

			case 'B' :
				bindcompat = TRUE;
				new_res.options |= RES_DNSRCH;
				break;

			case 'C' :
				checkmode = TRUE;
				listmode = TRUE;
				if (querytype == T_NONE)
					querytype = -1;	/* suppress zone data output */
				break;

			case 'c' :
				if (is_empty(argv[2]) || argv[2][0] == '-')
					usage_error("Missing query class");
				queryclass = parse_class(argv[2]);
				if (queryclass < 0)
					usage_error("Invalid query class %s", argv[2]);
				argv++; argc--;
				break;

			case 'd' :
				debug++;		/* increment debugging level */
				new_res.options |= RES_DEBUG;
				break;

			case 'e' :
				exclusive = TRUE;
				break;

			case 'F' :
				logexchange = TRUE;
				/*FALLTHROUGH*/

			case 'f' :
				if (is_empty(argv[2]) || argv[2][0] == '-')
					usage_error("Missing log file name");
				logfilename = argv[2];
				argv++; argc--;
				break;

#ifdef justfun
			case 'g' :
				namelen = getval(argv[2], "minimum length", 1, MAXDNAME);
				argv++; argc--;
				break;
#endif

			case 'D' :
			case 'E' :
			case 'G' :
			case 'H' :
				if (*option == 'D')
					duplmode = TRUE;
				if (*option == 'E')
					extrmode = TRUE;
				if (*option == 'G')
					gatemode = TRUE;
				hostmode = TRUE;
				listmode = TRUE;
				if (querytype == T_NONE)
					querytype = -1;	/* suppress zone data output */
				break;

			case 'h':
				printf("%s", Help);
				exit(EX_OK);
				break;

			case 'I' :
				if (argv[2] == NULL || argv[2][0] == '-')
					usage_error("Missing allowed chars");
				invalid = argv[2];
				argv++; argc--;
				break;

			case 'i' :
				reverse = TRUE;
				break;

			case 'J':
				maxport = getval(argv[2], "last port number",
						 (minport > 0) ? minport : 1, MAXINT16);
				if (minport == 0)
					minport = maxport;
				argc--, argv++;
				break;

			case 'j':
				minport = getval(argv[2], "first port number",
						 1, (maxport > 0) ? maxport : MAXINT16);
				if (maxport == 0)
					maxport = minport;
				argc--, argv++;
				break;

			case 'K' :
				timing = TRUE;
				break;

			case 'L' :
				recursive = getval(argv[2], "recursion level", 1, 0);
				argv++; argc--;
				/*FALLTHROUGH*/

			case 'l' :
				listmode = TRUE;
				classprint = TRUE;
				break;

			case 'M' :
				mxdomains = TRUE;
				listmode = TRUE;
				if (querytype == T_NONE)
					querytype = -1;	/* suppress zone data output */
				break;

			case 'm' :
				mailmode = TRUE;
				querytype = T_MAILB;	/* filter MINFO/MG/MR/MB data */
				break;

			case 'N' :
				if (is_empty(argv[2]) || argv[2][0] == '-')
					usage_error("Missing zone to be skipped");
				skipzone = argv[2];
				argv++; argc--;
				break;

			case 'n' :
				revnsap = TRUE;
				break;

			case 'O' :
				if (is_empty(argv[2]) || argv[2][0] == '-')
					usage_error("Missing source address");
				srcaddr = inet_addr(argv[2]);
				if (srcaddr == NOT_DOTTED_QUAD)
					usage_error("Invalid source address %s", argv[2]);
				argv++; argc--;
				break;

			case 'o' :
				suppress = TRUE;
				break;

			case 'P' :
				parent = TRUE;
				break;

			case 'p' :
				primary = TRUE;
				break;

			case 'Q' :
				quick = TRUE;
				break;

			case 'q' :
				quiet = TRUE;
				break;

			case 'R' :
				new_res.options |= RES_DNSRCH;
				break;

			case 'r' :
				norecurs = TRUE;
				new_res.options &= ~((unsigned) RES_RECURSE);
				break;

			case 'S' :
				statistics = TRUE;
				break;

			case 's' :
				new_res.retrans = getval(argv[2], "timeout value", 1, 0);
				argv++; argc--;
				break;

			case 'T' :
				ttlprint = TRUE;
				break;

			case 't' :
				if (is_empty(argv[2]) || argv[2][0] == '-')
					usage_error("Missing query type");
				querytype = parse_type(argv[2]);
				if (querytype < 0)
					usage_error("Invalid query type %s", argv[2]);
				argv++; argc--;
				break;

			case 'u' :
				new_res.options |= RES_USEVC;
				break;

			case 'v' :
				verbose++;		/* increment verbosity level */
				break;

			case 'W' :
				wildcards = TRUE;
				listmode = TRUE;
				if (querytype == T_NONE)
					querytype = T_MX;
				break;

			case 'w' :
				waitmode = TRUE;
				break;

			case 'X' :
				if (is_empty(argv[2]) || argv[2][0] == '-')
					usage_error("Missing server name");
				servername = argv[2];
				argv++; argc--;
				/* FALLTHROUGH */

			case 'x' :
				extended = TRUE;
				break;

			case 'Y' :
				dumpdata = TRUE;
				break;

			case 'Z' :
				dotprint = TRUE;
				ttlprint = TRUE;
				classprint = TRUE;
				break;

			case 'z' :
				listzones = TRUE;
				listmode = TRUE;
				if (querytype == T_NONE)
					querytype = -1;	/* suppress zone data output */
				break;

			case 'V' :
#if defined(__NAMESER) && !defined(HOST_RES_SEND)
				printf("Host version %s, BIND-8 resolver version %d, API version: %d\n", version, __RES, __NAMESER);
#elif defined(__BIND) && !defined(HOST_RES_SEND)
				printf("Host version %s, BIND-4 resolver version %d, API version: %d\n", version, __RES, __BIND);
#elif defined(BIND_4_8) && !defined(HOST_RES_SEND)
				printf("Host version %s, BIND 4.8.x resolver version %d\n", version, __RES);
#elif defined(__NAMESER) && defined(HOST_RES_SEND)
# include "ERROR: -DHOST_RES_SEND will not work with BIND-8"
				printf("Host version %s, using private res_send() with BIND-8 resolver version %d, API version %d\n", version, __RES, __NAMESER);
#elif defined(__BIND) && defined(HOST_RES_SEND)
				printf("Host version %s, using private res_send() with BIND-4 resolver version %d, API version %d\n", version, __RES, __BIND);
#elif defined(BIND_4_8) && defined(HOST_RES_SEND)
				printf("Host version %s, using private res_send() with BIND 4.8.x resolver version %d\n", version, __RES);
#elif defined(HOST_RES_SEND)
				printf("Host version %s, using private res_send() with very old or non-BIND headers\n", version);
#else
				printf("Host version %s, unknown resolver version\n", version);
#endif
				exit(EX_OK);
				/* NOTREACHED */

			case '_':
				undercheck = TRUE;
				break;

			default:
				usage_error(usage_msg);
			}
		}

		argv++; argc--;
	}

	/*
	 * Check the remaining arguments.
	 */
	/* old syntax must have at least one argument and no more than two */
	if (!extended && (argc < 2 || argv[1] == NULL || argc > 3))
		usage_error(usage_msg);

	/* old syntax has explicit server as second argument */
	if (!extended && (argc > 2 && argv[2] != NULL))
		servername = argv[2];

	/*
	 * set defaults from major modes
	 */
	if (listmode && !canoncheck)
		canonskip = TRUE;	/* assume --canonskip was given if in
					 * any zone listing mode, unless an
					 * explicit --canoncheck was also
					 * specified
					 */

	/*
	 * Check for incompatible options.
	 */
	if ((querytype < 0) && !listmode)
		usage_error("No query type specified");

	if (loadzone && (servername != NULL))
		usage_error("Conflicting options --load and explicit server");

	if (parent && (servername != NULL))
		usage_error("Conflicting options -P and explicit server");

	if (loadzone && dumpzone)
		usage_error("Conflicting options --load and --dump");

	if (canoncheck && canonskip)
		usage_error("Conflicting options --canonskip and --canoncheck");

	/*
	 * Open log file if requested.
	 */
	if (logfilename != NULL)
		set_logfile(logfilename);

	/*
	 * Move to cache directory if specified.
	 */
	if (cachedirname != NULL)
		set_cachedir(cachedirname);

	/*
	 * Set default preferred server for zone listings, if not specified.
	 */
	if (listmode && !checkmode && !prefserver && !servername) {
		if (!(prefserver = myhostname()))
			fprintf(stderr, "Nameserver selection will be random, use -P for more control\n");
	}

	/*
	 * Check for possible alternative server. Use new resolver defaults.
	 */
	if (servername != NULL)
		set_server(servername);

	/*
	 * Do final resolver initialization.
	 * Show resolver parameters and special environment options.
	 */
	/* set new resolver values changed by command options */
	_res.retry = new_res.retry;
	_res.retrans = new_res.retrans;
	_res.options = new_res.options;

	/* show the new resolver database */
	if (!parent && (verbose > 1 || debug > 1))
		show_res();

	/* show customized default domain */
	option = getenv("LOCALDOMAIN");
	if (option != NULL && verbose > 1)
		printf("Explicit local domain %s\n\n", option);

	/*
	 * Process command line argument(s) depending on syntax.
	 */
	pr_timestamp("host starting");

	if (!extended)			/* only one argument */
		result = process_name(argv[1]);
	else if (argc < 2)		/* no arguments */
		result = process_file(stdin);
	else				/* multiple command line arguments */
		result = process_argv(argc, argv);

	pr_timestamp("host finished");

	/*
	 * Report result status of action taken.
	 */
	exit(result);
	/*NOTREACHED*/
}

/*
** SET_DEFAULTS -- Interpolate default options and parameters in argv
** ------------------------------------------------------------------
**
**	The HOST_DEFAULTS environment variable gives customized options.
**
**	Returns:
**		None.
**
**	Outputs:
**		Creates ``optargv'' vector with ``optargc'' arguments.
*/

void
set_defaults(option, argc, argv)
	input char *option;		/* option string */
	input int argc;			/* original command line arg count */
	input char *argv[];		/* original command line arguments */
{
	register char *p, *q;
	register int i;

	/*
	 * Allocate new argument vector.
	 */
	optargv = newlist(NULL, 2, char *);
	optargv[0] = argv[0];
	optargc = 1;

	/*
	 * Construct argument list from option string.
	 */
	for (q = newstr(option), p = q; *p != '\0'; p = q) {
		while (is_space((int) *p))
			p++;

		if (*p == '\0')
			break;

		for (q = p; *q != '\0' && !is_space((int) *q); q++)
			continue;

		if (*q != '\0')
			*q++ = '\0';

		optargv = newlist(optargv, optargc+2, char *);
		optargv[optargc] = p;
		optargc++;
	}

	/*
	 * Append command line arguments.
	 */
	for (i = 1; i < argc && argv[i] != NULL; i++) {
		optargv = newlist(optargv, optargc+2, char *);
		optargv[optargc] = argv[i];
		optargc++;
	}

	/* and terminate */
	optargv[optargc] = NULL;

	return;
}

/*
** GETVAL -- Decode parameter value and perform range check
** --------------------------------------------------------
**
**	Returns:
**		Parameter value if successfully decoded.
**		Aborts in case of syntax or range errors.
*/

int
getval(optstring, optname, minvalue, maxvalue)
	input char *optstring;		/* parameter from command line */
	input char *optname;		/* descriptive name of option */
	input int minvalue;		/* minimum value for option */
	input int maxvalue;		/* maximum value for option */
{
	register int optvalue;

	if (is_empty(optstring) || optstring[0] == '-')
		usage_error("Missing %s", optname);

	optvalue = atoi(optstring);

	if (optvalue == 0 && optstring[0] != '0')
		usage_error("Invalid %s %s", optname, optstring);

	if (optvalue < minvalue)
		usage_error("Minimum %s %s", optname, dtoa(minvalue));

	if (maxvalue > 0 && optvalue > maxvalue)
		usage_error("Maximum %s %s", optname, dtoa(maxvalue));

	return (optvalue);
}

/*
** CVTOPT -- Convert long option syntax
** ------------------------------------
**
**	Returns:
**		Short option string, if appropriate.
*/

char *
cvtopt(optstring)
	input char *optstring;		/* parameter from command line */
{
	register char *value;

	/* separate keyword and value */
	if ((value = strchr(optstring, '=')))
		*value++ = '\0';

	/*
	 * These are just alternatives for short options.
	 */
	if (sameword(optstring, "addrcheck"))
		return ("-A");

	if (sameword(optstring, "anything"))
		return ("-a");

	if (sameword(optstring, "checksoa"))
		return ("-C");

	if (sameword(optstring, "debug"))
		return ("-d");

	if (sameword(optstring, "exclusive"))
		return ("-e");

	if (sameword(optstring, "extended"))
		return ("-x");

	if (sameword(optstring, "full"))
		return ("-Z");

	if (sameword(optstring, "help"))
		return ("-h");

	if (sameword(optstring, "hostcount"))
		return ("-H");

	if (sameword(optstring, "inaddr"))
		return ("-i");

	if (sameword(optstring, "list"))
		return ("-l");

	if (sameword(optstring, "norecurs"))
		return ("-r");

	if (sameword(optstring, "parent"))
		return ("-P");

	if (sameword(optstring, "primary"))
		return ("-p");

	if (sameword(optstring, "quick"))
		return ("-Q");

	if (sameword(optstring, "quiet"))
		return ("-q");

	if (sameword(optstring, "statistics"))
		return ("-S");

	if (sameword(optstring, "suppress"))
		return ("-o");

	if (sameword(optstring, "tcp"))
		return ("-u");

	if (sameword(optstring, "verbose"))
		return ("-v");

	if (sameword(optstring, "version"))
		return ("-V");

	if (sameword(optstring, "undercheck"))
		return ("-_");

	/*
	 * alternatives for short options with values
	 * (parsed and set here because of the '=' syntax)
	 */

	if (sameword(optstring, "class")) { /* also -c */
		if (is_empty(value) || value[0] == '-')
			usage_error("Missing query class");
		queryclass = parse_class(value);
		if (queryclass < 0)
			usage_error("Invalid query class %s", value);
		return ("-");
	}

	if (sameword(optstring, "depth")) { /* also -L */
		recursive = getval(value, "recursion level", 1, 0);
		return ("-l");
	}

	if (sameword(optstring, "file")) { /* also -f */
		if (is_empty(value) || value[0] == '-')
			usage_error("Missing log file name");
		logfilename = value;
		return ("-");
	}

	if (sameword(optstring, "server")) { /* also -X */
		if (is_empty(value) || value[0] == '-')
			usage_error("Missing server name");
		servername = value;
		return ("-");
	}

	if (sameword(optstring, "timeout")) { /* also -s */
		new_res.retrans = getval(value, "timeout value", 1, 0);
		return ("-");
	}

	if (sameword(optstring, "type")) { /* also -t */
		if (is_empty(value) || value[0] == '-')
			usage_error("Missing query type");
		querytype = parse_type(value);
		if (querytype < 0)
			usage_error("Invalid query type %s", value);
		return ("-");
	}

	/*
	 * Combinations of several short options.
	 */

	if (sameword(optstring, "checkzone")) {
		if (recursive == 0)
			recursive = 1;
		canoncheck = TRUE;
		checkmode = TRUE;
		listmode = TRUE;
		addrmode = TRUE;
		if (querytype == T_NONE)
			querytype = -1;	/* suppress zone data output */
		return ("-");
	}

	/*
	 * New long options without an equivalent short one.
	 */

	if (sameword(optstring, "canoncheck")) {
		canoncheck = TRUE;
		return ("-");
	}

	if (sameword(optstring, "canonskip")) {
		canonskip = TRUE;
		return ("-");
	}

	if (sameword(optstring, "cnamecheck")) {
		cnamecheck = TRUE;
		return ("-");
	}

	if (sameword(optstring, "compare")) { /* -k ? */
		if (value != NULL) {
			time_t now = time((time_t *) NULL);
			int period = convtime(value, 'd');

			if (period < 0 || period > now)
				usage_error("Invalid time period %s", value);

			/* set absolute time in the past */
			loadtime = now - period;
		}

		compare = TRUE;
		return ("-");
	}

	if (sameword(optstring, "dump")) {
		cachedirname = value;	/* optional */
		dumpzone = TRUE;
		return ("-l");
	}

	if (sameword(optstring, "load")) {
		cachedirname = value;	/* optional */
		loadzone = TRUE;
		return ("-l");
	}

	if (sameword(optstring, "nothing")) {
		querytype = -1;
		return ("-");
	}

	if (sameword(optstring, "prefserver")) {
		if (is_empty(value) || value[0] == '-')
			usage_error("Missing prefserver name");
		prefserver = value;
		return ("-");
	}

	if (sameword(optstring, "recursive")) {	/* -y ? */
		if (recursive == 0)
			recursive = MAXINT16;
		return ("-l");
	}

	if (sameword(optstring, "retry")) {
		new_res.retry = getval(value, "retry count", 1, 0);
		return ("-");
	}

	if (sameword(optstring, "test")) {
		testmode = TRUE;	/* just calls test.c:test(), for testing new code. */
		return ("-");
	}

	if (sameword(optstring, "usage")) {
		printf("%s\n", usage_msg);
		exit(EX_OK);
	}

	if (*optstring != '\0')
		usage_error("Unrecognized option %s", optstring);

	/* just ignore */
	return ("-");
}

/*
** PROCESS_ARGV -- Process command line arguments
** ----------------------------------------------
**
**	Returns:
**		EX_OK if information was obtained successfully.
**		Appropriate exit code otherwise.
*/

int
process_argv(argc, argv)
	input int argc;
	input char *argv[];
{
	register int i;
	int result;			/* result status of action taken */
	int excode = EX_NOINPUT;	/* overall result status */

	for (i = 1; i < argc && argv[i] != NULL; i++) {
		/* process a single argument */
		result = process_name(argv[i]);

		/* maintain overall result */
		if (result != EX_OK || excode == EX_NOINPUT)
			excode = result;
	}

	/* return overall result */
	return (excode);
}

/*
** PROCESS_FILE -- Process arguments from input file
** -------------------------------------------------
**
**	Returns:
**		EX_OK if information was obtained successfully.
**		Appropriate exit code otherwise.
*/

int
process_file(fp)
	input FILE *fp;			/* input file with query names */
{
	register char *p, *q;
	char buf[BUFSIZ];
	int result;			/* result status of action taken */
	int excode = EX_NOINPUT;	/* overall result status */

	while (fgets(buf, (int) sizeof(buf), fp) != NULL) {
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = '\0';

		/* extract names separated by whitespace */
		for (q = buf, p = q; *p != '\0'; p = q) {
			while (is_space((int) *p))
				p++;

			/* ignore comment lines */
			if (*p == '\0' || *p == '#' || *p == ';')
				break;

			for (q = p; *q != '\0' && !is_space((int) *q); q++)
				continue;

			if (*q != '\0')
				*q++ = '\0';

			/* process a single argument */
			result = process_name(p);

			/* maintain overall result */
			if (result != EX_OK || excode == EX_NOINPUT)
				excode = result;
		}
	}

	/* return overall result */
	return (excode);
}

/*
** PROCESS_NAME -- Process a single command line argument
** ------------------------------------------------------
**
**	Returns:
**		EX_OK if information was obtained successfully.
**		Appropriate exit code otherwise.
**
**	Wrapper for execute_name() to hide administrative tasks.
*/

int
process_name(name)
	input char *name;		/* command line argument */
{
	static int save_querytype;
	static bool_t save_reverse;
	static bool_t firstname = TRUE;

	/* separate subsequent pieces of output */
	if (!firstname && (verbose || debug || checkmode))
		printf("\n");

	/*
	 * Some global variables are redefined further on.  Save their initial
	 * values in the first pass, and restore them during subsequent passes.
	 */
	if (firstname) {
		save_querytype = querytype;
		save_reverse = reverse;
		firstname = FALSE;
	} else {
		querytype = save_querytype;
		reverse = save_reverse;
	}

	/*
	 * Do the real work.
	 */
	return execute_name(name);
}

/*
** EXECUTE_NAME -- Process a single command line argument
** ------------------------------------------------------
**
**	Returns:
**		EX_OK if information was obtained successfully.
**		Appropriate exit code otherwise.
**
**	Outputs:
**		Defines ``queryname'' and ``queryaddr'' appropriately.
**
**	Side effects:
**		May redefine ``querytype'' and ``reverse'' if necessary.
*/

int
execute_name(name)
	input char *name;		/* command line argument */
{
	bool_t result;			/* result status of action taken */
#ifdef HAVE_INET_ATON
	struct in_addr inaddr;
#endif

	/* check for nonsense input name */
	if (strlength(name) > MAXDNAME) {
		errmsg("Query name %s too long", name);
		return (EX_USAGE);
	}

	/*
	 * Analyze the name and type to be queried about.
	 * The name can be an ordinary domain name, or an internet address
	 * in dotted quad notation. If the -n option is given, the name is
	 * supposed to be a dotted nsap address.
	 * Furthermore, an empty input name is treated as the root domain.
	 */
	queryname = name;
	if (queryname[0] == '\0')
		queryname = ".";

#ifdef HAVE_INET_ATON
	if (inet_aton(queryname, &inaddr))
		queryaddr = inaddr.s_addr;
	else
		queryaddr = NOT_DOTTED_QUAD;
#else
	/*
	 * XXX are some inet_addr(3) implementations so broken they parse "."
	 * wrongly and see it as some number?
	 */
	if (sameword(queryname, "."))
		queryaddr = NOT_DOTTED_QUAD;
	else
		queryaddr = inet_addr(queryname);
#endif

	/*
	 * Generate reverse in-addr.arpa query if so requested.
	 * The input name must be a dotted quad, and be convertible.
	 */
	if (reverse) {
		if (queryaddr == NOT_DOTTED_QUAD)
			name = NULL;
		else
			name = in_addr_arpa(queryname);

		if (name == NULL) {
			errmsg("Invalid dotted quad %s", queryname);
			return (EX_USAGE);
		}

		/* redefine appropriately */
		queryname = name;
		queryaddr = NOT_DOTTED_QUAD;
	}

	/*
	 * Heuristic to check whether we are processing a reverse mapping domain.
	 * Normalize to not have trailing dot, unless it is the root zone.
	 */
	if ((queryaddr == NOT_DOTTED_QUAD) && !reverse) {
		char namebuf[MAXDNAME+1];
		register int n;

		name = strcpy(namebuf, queryname);

		n = strlength(name);
		if (n > 1 && name[n-1] == '.')
			name[n-1] = '\0';

		reverse = indomain(name, ARPA_ROOT, FALSE);
	}

	/*
	 * Generate reverse nsap.int query if so requested.
	 * The input name must be a dotted nsap, and be convertible.
	 */
	if (revnsap) {
		if (reverse)
			name = NULL;
		else
			name = nsap_int(queryname);

		if (name == NULL) {
			errmsg("Invalid nsap address %s", queryname);
			return (EX_USAGE);
		}

		/* redefine appropriately */
		queryname = name;
		queryaddr = NOT_DOTTED_QUAD;

		/* this is also a reversed mapping domain */
		reverse = TRUE;
	}
	/* must have regular name or dotted quad in addrmode */
	if (!listmode && addrmode && reverse) {
		errmsg("Do not use '-i' with '-A'.");
		return (EX_USAGE);
	}

	/*
	 * In regular mode, the querytype is used to formulate the nameserver
	 * query, and any response is filtered out when processing the answer.
	 * In listmode, the querytype is used to filter out the proper records.
	 */
	/* set querytype for regular mode if unspecified */
	if ((querytype == T_NONE) && !listmode) {
		if ((queryaddr != NOT_DOTTED_QUAD) || reverse)
			querytype = T_PTR;
		else if (parent)
			querytype = T_NS;
		else
			querytype = T_A;
	}

	/*
	 * Check for incompatible options.
	 */
	/* cannot have dotted quad in listmode */
	if (listmode && (queryaddr != NOT_DOTTED_QUAD)) {
		errmsg("Invalid query name %s (dotted quad not allowed in listmode)", queryname);
		return (EX_USAGE);
	}

	/* must have plain name with -P */
	if (parent && queryaddr != NOT_DOTTED_QUAD) {
		errmsg("Invalid query name %s (need a plain name with -P)", queryname);
		return (EX_USAGE);
	}

	/* show what we are going to query about */
	if (verbose)
		show_types(queryname, querytype, queryclass);

	/*
	 * All set. Perform requested function.
	 */
	result = execute(queryname, queryaddr);

	return (result ? EX_OK : EX_UNAVAILABLE);
}

/*
** EXECUTE -- Perform the requested function
** -----------------------------------------
**
**	Returns:
**		TRUE if information was obtained successfully.
**		FALSE otherwise.
**
**	The whole environment has been set up and checked.
*/

bool_t
execute(name, addr)
	input char *name;		/* name to query about */
	input ipaddr_t addr;		/* explicit address of query */
{
	bool_t result;			/* result status of action taken */

	/*
	 * Special mode to test code separately.
	 */
	if (testmode) {
		result = test(name, addr);
		return (result);
	}

	/*
	 * Special mode to list contents of specified zone.
	 *
	 * Handling of primary and parent is done in list.c
	 */
	if (listmode) {
		result = list_zone(name);
		return (result);
	}

	/*
	 * If we're not doing test() or list_zone() then interpret '-p' as if the
	 * primary server were given as the servername (i.e. as -X or second arg).
	 */
	if (primary && !parent) {
		char *primaryname;

		if (!(primaryname = get_primary(name))) {
			ns_error(name, T_NS, queryclass, server);
			return (FALSE);
		}
		set_server(primaryname);
	} else if (parent) {
		/*
		 * XXX this code should be refactored out into a separate
		 * function so that the similar code in list.c:get_servers()
		 * can also be refactored out and use the same function.
		 */
		/*
		 * If -P was specified we try using the parent zone's
		 * nameservers instead of whatever's configured for the local
		 * resolver.  There's not a whole heck of a lot of utility for
		 * this particular feature other than to ask the first
		 * available parent server for NS records (and any incidental
		 * glue records).
		 */
		char pzn[MAXDNAME + 2];
		char *last_dot;
		char *parent_zone;
		int save_verbose;

		/* there is always one final parent:  "." */
		assert(strlen(name) <= MAXDNAME);
		last_dot = strrchr(name, '.');
		sprintf(pzn, "%s%s", name, (last_dot && *last_dot == '.' && *(last_dot+1) == '\0') ? "" : ".");
		parent_zone = pzn;

		if (verbose > 1)
			printf("(Trying to use the parent nameservers for %s)\n", name);

		save_verbose = verbose;
		if (verbose)
			verbose--;

		/*
		 * since this code mimics similar code enabled by 'parent' in
		 * list.c:get_servers(), which we call via use_servers(), we
		 * must turn off the parent flag when calling use_servers()!
		 */
		parent = FALSE;

		/*
		 * initially we must force recursion on in case it was turned
		 * off on the command line -- we'll be asking the default
		 * resolv.conf nameserver(s) for these NS records, and it will
		 * likely have to do a recursive lookup
		 */
		_res.options |= RES_RECURSE;

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
				printf("(Searching for nameservers for the parent zone: %s)\n", parent_zone);
			if (use_servers(parent_zone) != FALSE) {
				parent = TRUE; /* used to indicate success */
				break;
			}
			/* make sure next call to use_servers works right! */
			server = NULL;
			/* don't loop forever.... */
			if (sameword(parent_zone, "."))
				break;
			/* restore any trimmed trailing dot in case it's the last one */
			if (dot_offset)
				parent_zone[dot_offset] = '.';
		}
		verbose = save_verbose;
		if (!parent) {
			ns_error(name, T_NS, queryclass, (char *) NULL);
			return (FALSE);
		}
		/*
		 * turn off nameserver recursion for queries using the
		 * nameservers discovered through the parent zone delegation,
		 * also disable canonical checks of NS and MX records that
		 * might be retrieved subsequently -- the nameservers reported
		 * above from the parent zone should all be authoritative and
		 * may (should!) also be non-recursive.
		 */
		_res.options &= ~((unsigned) RES_RECURSE);
		canonskip = TRUE;

		if (verbose > 1 || debug > 1)
			show_res();
	}

	/*
	 * Special mode to check reverse mappings of host addresses.
	 */
	if (addrmode) {
		if (addr == NOT_DOTTED_QUAD)
			result = check_addr(name);
		else
			result = check_name(addr);
		return (result);
	}

	/*
	 * Regular mode to query about specified host.
	 */
	return host_query(name, addr);
}

/*
** HOST_QUERY -- Regular mode to query about specified host
** --------------------------------------------------------
**
**	Returns:
**		TRUE if information was obtained successfully.
**		FALSE otherwise.
*/

bool_t
host_query(name, addr)
	input char *name;		/* name to query about */
	input ipaddr_t addr;		/* explicit address of query */
{
	struct hostent *hp;
	struct in_addr inaddr;
	char newnamebuf[MAXDNAME+1];
	char *newname = NULL;		/* name to which CNAME is aliased */
	int ncnames = 0;		/* count of CNAMEs in chain */
	bool_t result;			/* result status of action taken */

	inaddr.s_addr = addr;

	result = FALSE;
	set_h_errno(TRY_AGAIN);

	/* retry until positive result or permanent failure */
	while (result == FALSE && h_errno == TRY_AGAIN) {
		/* reset before each query to avoid stale data */
		set_errno(0);
		realname = NULL;

		if (addr == NOT_DOTTED_QUAD) {
			/* reset CNAME indicator */
			cname = NULL;

			/* lookup the name in question */
			if (newname == NULL)
				result = get_hostinfo(name, FALSE);
			else
				result = get_hostinfo(newname, TRUE);

			/* recurse on CNAMEs, but not too deep */
			if (cnamecheck && cname && (querytype != T_CNAME)) {
				newname = strcpy(newnamebuf, cname);

				if (ncnames++ > MAXCHAIN) {
					errmsg("Possible CNAME loop");
					return (FALSE);
				}

				result = FALSE;
				set_h_errno(TRY_AGAIN);
				continue;
			}
		} else {
			if (querytype != T_NONE && querytype != T_ANY && querytype != T_PTR) {
				errmsg("Invalid query type (%s) for use with geth_byaddr() (try adding '-i')", pr_type(querytype));
				return FALSE;
			}
			if ((hp = geth_byaddr((char *) &inaddr, (socklen_t) INADDRSZ, AF_INET))) {
				print_host("Name", hp);
				result = TRUE;
				geth_freehostent(hp);
			}
		}

		/* only retry if so requested */
		if (!waitmode)
			break;
	}

	/* use actual name if available */
	if (realname != NULL)
		name = realname;

	/* explain the reason of a failure */
	if (result == FALSE)
		ns_error(name, querytype, queryclass, server);

	return (result);
}

/*
** MYHOSTNAME -- Determine our own fully qualified host name
** ---------------------------------------------------------
**
**	Returns:
**		Pointer to own host name.
**		Aborts if host name could not be determined.
*/

char *
myhostname()
{
	struct hostent *hp;
	static char mynamebuf[MAXDNAME+1];
	static char *myname = NULL;

	if (myname == NULL) {
		if (gethostname(mynamebuf, (size_t) MAXDNAME) < 0) {
			perror("gethostname");
			exit(EX_OSERR);
		}
		mynamebuf[MAXDNAME] = '\0';

		if (!(hp = gethostbyname(mynamebuf))) {
			ns_error(mynamebuf, T_A, C_IN, server);
			errmsg("Error in looking up own name");
			return (NULL);
		}

		/* cache the result */
		myname = strncpy(mynamebuf, hp->h_name, (size_t) MAXDNAME);
		myname[MAXDNAME] = '\0';
	}

	return (myname);
}

/*
** SET_SERVER -- Override default nameserver with explicit server
** --------------------------------------------------------------
**
**	Returns:
**		None.
**		Aborts the program if an unknown host was given.
**
**	Side effects:
**		The global variable ``server'' is set to indicate
**		that an explicit server is being used.
**
**	The default nameserver addresses in the resolver database
**	which are initialized by res_init() from /etc/resolv.conf
**	are replaced with the (possibly multiple) addresses of an
**	explicitly named server host. If a dotted quad is given,
**	only that single address will be used.
**
**	The answers from such server must be interpreted with some
**	care if we don't know beforehand whether it can be trusted.
*/

void
set_server(name)
	input char *name;		/* name of server to be queried */
{
	register int i;
	struct hostent *hp;
	struct in_addr inaddr;
	ipaddr_t addr;			/* explicit address of server */

	/* check for nonsense input name */
	if (strlength(name) > MAXDNAME) {
		errmsg("Server name %s too long", name);
		exit(EX_USAGE);
	}

	/*
	 * Overrule the default nameserver addresses.
	 */
	addr = inet_addr(name);
	inaddr.s_addr = addr;

	if (addr == NOT_DOTTED_QUAD) {
		/* lookup all of its addresses; this must not fail */
		if (!(hp = gethostbyname(name))) {
			ns_error(name, T_A, C_IN, server);
			errmsg("Error in looking up server name");
			exit(EX_NOHOST);
		}
		_res.nscount = 0;
		for (i = 0; _res.nscount < MAXNS && hp->h_addr_list[i]; i++, _res.nscount++) {
			nslist(_res.nscount).sin_family = AF_INET;
			nslist(_res.nscount).sin_port = htons(NAMESERVER_PORT);
			nslist(_res.nscount).sin_addr = incopy(hp->h_addr_list[i]);
		}
	} else {
		/* lookup the name, but use only the given address */
		hp = gethostbyaddr((char *) &inaddr, INADDRSZ, AF_INET);

		nslist(0).sin_family = AF_INET;
		nslist(0).sin_port = htons(NAMESERVER_PORT);
		nslist(0).sin_addr = inaddr;
		_res.nscount = 1;
	}

	/*
	 * Indicate the use of an explicit server.
	 */
	if (hp)	{
		server = strncpy(serverbuf, hp->h_name, (size_t) MAXDNAME);
		server[MAXDNAME] = '\0';

		if (verbose)
			print_host("Server", hp);
	} else {
		server = strcpy(serverbuf, inet_ntoa(inaddr));

		if (verbose)
			printf("Server: %s\n\n", server);
	}

	return;
}

/*
** SET_LOGFILE -- Initialize optional log file
** -------------------------------------------
**
**	Returns:
**		None.
**		Aborts the program if the file could not be created.
**
**	Side effects:
**		The global variable ``logfile'' is set to indicate
**		that resource record output is to be written to it.
**
**	Swap ordinary stdout and log file output if so requested.
*/

void
set_logfile(filename)
	input char *filename;		/* name of log file */
{
	if (logexchange) {
		if (!(logfile = fdopen(dup(STDOUT_FILENO), "w"))) {
			perror("fdopen");
			exit(EX_OSERR);
		}
		if (freopen(filename, "w", stdout) == NULL) {
			perror(filename);
			exit(EX_CANTCREAT);
		}
	} else {
		if (!(logfile = fopen(filename, "w"))) {
			perror(filename);
			exit(EX_CANTCREAT);
		}
	}

	return;
}

/*
** SET_CACHEDIR -- Move to the local cache directory
** -------------------------------------------------
**
**	Returns:
**		None.
**		Aborts the program if the cache is unavailable.
**
**	Side effects:
**		Changes the current working directory to the cache.
**		Cache files are relative to the top of the cache.
*/

void
set_cachedir(filename)
	input char *filename;		/* name of cache directory */
{
	if (chdir(filename) < 0) {
		cache_perror("Cannot chdir", filename);
		exit(EX_CANTCREAT);
	}

	return;
}

/*
** FATAL -- Abort program when an invalid option is encountered
** ------------------------------------------------------------
**
**	Returns:
**		Aborts after issuing error message.
*/

/*VARARGS1*/
static void
#ifdef __STDC__
usage_error(const char *fmt, ...)
#else
usage_error(fmt, va_alist)
	input const char *fmt;		/* format of message */
	va_dcl				/* arguments for printf */
#endif
{
	va_list ap;

	VA_START(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (fmt[strlen(fmt) - 1] != '\n')
		(void) fputc('\n', stderr);

	exit(EX_USAGE);
}
