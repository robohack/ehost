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

#ident "@(#)host:$Name:  $:$Id: send.c,v 1.13 2003-04-06 03:10:44 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)send.c	e07@nikhef.nl (Eric Wassenaar) 991331";
#endif

#include "host.h"

char *debug_prefix = DEBUG_PREFIX; /* prefix for debug messages to stdout */

ipaddr_t srcaddr = INADDR_ANY;	/* explicit source ip address */

int minport = 0;		/* first source port in explicit range */
int maxport = 0;		/* last  source port in explicit range */

unsigned int timeout;		/* connection read timeout */

static struct sockaddr_in from;	/* address of inbound packet */
static struct sockaddr *from_sa = (struct sockaddr *) &from;

#ifdef HOST_RES_SEND

static int srvsock = -1;	/* socket descriptor */

/*
** RES_SEND -- Send nameserver query and retrieve answer
** -----------------------------------------------------
**
**	Returns:
**		Length of (untruncated) nameserver answer, if obtained.
**		-1 if an error occurred (errno set appropriately).
**
**	This is a simplified version of the BIND 4.8.3 res_send().
**	- Always use connected datagrams to get proper error messages.
**	- Do not only return ETIMEDOUT or ECONNREFUSED in datagram mode,
**	  but also host or network unreachable status if appropriate.
**	- Never leave a connection open after we got an answer.
**	- No special ECONNRESET handling when using virtual circuits.
**
**	Note that this private version of res_send() is not only called
**	directly by 'host' but also indirectly by gethostbyname() or by
**	gethostbyaddr() via their resolver interface routines.
*/

int
res_send(query, querylen, answer, anslen)
	input const qbuf_t *query;	/* location of formatted query buffer */
	input int querylen;		/* length of query buffer */
	output qbuf_t *answer;		/* location of buffer to store answer */
	input int anslen;		/* maximum size of answer buffer */
{
	HEADER *bp = (HEADER *) answer;
	struct sockaddr_in *addr;	/* the server address to connect to */
	int v_circuit;			/* virtual circuit or datagram switch */
	int servfail[MAXNS];		/* saved failure codes per nameserver */
	register int try, ns;
	register int n;

	/* make sure resolver has been initialized */
	if (!bitset(RES_INIT, _res.options) && res_init() == -1)
		return (-1);

	if (bitset(RES_DEBUG, _res.options)) {
		printf("%sres_send()\n", debug_prefix);
		pr_query(query, querylen, stdout);
	}

	/* use virtual circuit if requested or if necessary */
	v_circuit = bitset(RES_USEVC, _res.options) || (querylen > PACKETSZ);

	/* reset server failure codes */
	for (ns = 0; ns < MAXNS; ns++)
		servfail[ns] = 0;

	/*
	 * Do _res.retry attempts for each of the _res.nscount addresses.
	 * Upon failure, the current server will be marked bad if we got
	 * an error condition which makes it unlikely that we will succeed
	 * the next time we try this server.
	 * Internal operating system failures, such as temporary lack of
	 * resources, do not fall in that category.
	 */
	for (try = 0; try < _res.retry; try++) {
		for (ns = 0; ns < _res.nscount; ns++) {
			/* skip retry if server failed permanently */
			if (servfail[ns])
				continue;

			/* fetch server address */
			addr = &nslist(ns);
retry:
			if (bitset(RES_DEBUG, _res.options)) {
				printf("%sQuerying server (# %d) %s address = %s, accepting up to %d answer bytes\n",
				       debug_prefix, ns+1, v_circuit ? "tcp" : "udp",
				       inet_ntoa(addr->sin_addr),
				       anslen);
			}
			if (v_circuit) {
				/* at most one attempt per server */
				try = _res.retry;

				/* connect via virtual circuit */
				n = send_stream(addr, query, querylen, answer, anslen);
			} else {
				/* set datagram read timeout for recv_sock() */
				timeout = (_res.retrans << try);
				if (try > 0)
					timeout /= _res.nscount;
				if (timeout <= 0)
					timeout = 1;

				/* connect via datagram */
				n = send_dgram(addr, query, querylen, answer, anslen);

				/* check truncation; use v_circuit with same server */
				if ((n > 0) && bp->tc) {
					if (bitset(RES_DEBUG, _res.options)) {
						printf("%struncated answer, %d bytes\n",
						       debug_prefix, n);
					}
					if (!bitset(RES_IGNTC, _res.options)) {
						v_circuit = 1;
						goto retry;
					}
				}
			}
			if (n <= 0) {
				switch (errno) {
				case ECONNREFUSED:
				case ENETDOWN:
				case ENETUNREACH:
				case EHOSTDOWN:
				case EHOSTUNREACH:
					servfail[ns] = errno;
					break;
				}
				/* try next server */
				continue;
			}
			if (bitset(RES_DEBUG, _res.options)) {
				printf("%sgot answer, %d bytes:\n", debug_prefix, n);
				pr_query(answer, (n > anslen) ? anslen : n, stdout);
			}

			/* we have an answer; clear possible error condition */
			set_errno(0);
			return (n);
		}
	}

	/* no answer obtained; return error condition */
	return (-1);
}

/*
** CHECK_FROM -- Make sure the response comes from a known server
** --------------------------------------------------------------
**
**	Returns:
**		Nonzero if the source address is known.
**		Zero otherwise.
*/

static bool_t
check_from()
{
	struct sockaddr_in *addr;
	register int ns;

	for (ns = 0; ns < _res.nscount; ns++) {
		/* fetch server address */
		addr = &nslist(ns);

		if (from.sin_family != addr->sin_family)
			continue;

		if (from.sin_port != addr->sin_port)
			continue;

		/* this allows a reply from any responding server */
		if (addr->sin_addr.s_addr == INADDR_ANY)
			return (TRUE);

		if (from.sin_addr.s_addr == addr->sin_addr.s_addr)
			return (TRUE);
	}

	/* matches none of the known addresses */
	return (FALSE);
}

/*
** SEND_STREAM -- Query nameserver via virtual circuit
** ---------------------------------------------------
**
**	Returns:
**		Length of (untruncated) nameserver answer, if obtained.
**		-1 if an error occurred.
**
**	A new socket is allocated for each call, and it is never
**	left open. Checking the packet id is rather redundant.
**
**	Note that connect() is the call that is allowed to fail
**	under normal circumstances. All other failures generate
**	an unconditional error message.
**	Note that truncation is handled within host_res_read().
*/

static int
send_stream(addr, query, querylen, answer, anslen)
	input struct sockaddr_in *addr;	/* the server address to connect to */
	input qbuf_t *query;		/* location of formatted query buffer */
	input int querylen;		/* length of query buffer */
	output qbuf_t *answer;		/* location of buffer to store answer */
	input int anslen;		/* maximum size of answer buffer */
{
	char *host = NULL;		/* name of server is unknown */
	HEADER *qp = (HEADER *) query;
	HEADER *bp = (HEADER *) answer;
	register int n;

	/*
	 * Setup a virtual circuit connection.
	 */
	srvsock = host_res_socket(AF_INET, SOCK_STREAM, 0);
	if (srvsock < 0) {
		host_res_perror(addr, host, "socket");
		return (-1);
	}
	if (host_res_connect(srvsock, addr, sizeof(*addr)) < 0) {
		if (bitset(RES_DEBUG, _res.options))
			host_res_perror(addr, host, "connect");
		(void) host_res_close(srvsock);
		return (-1);
	}
	if (bitset(RES_DEBUG, _res.options)) {
		printf("%sconnected to %s\n",
			debug_prefix, inet_ntoa(addr->sin_addr));
	}
	/*
	 * Send the query buffer.
	 */
	if (host_res_write(srvsock, addr, host, (char *) query, querylen) < 0) {
		(void) host_res_close(srvsock);
		return (-1);
	}

	/*
	 * Read the answer buffer.
	 */
wait:
	if ((n = host_res_read(srvsock, addr, host, (char *) answer, anslen, 0)) < 0) {
		(void) host_res_close(srvsock);
		return (-1);
	}

	/*
	 * Make sure it is the proper response by checking the packet id.
	 */
	if (qp->id != bp->id) {
		if (bitset(RES_DEBUG, _res.options)) {
			printf("%sunexpected answer:\n", debug_prefix);
			pr_query(answer, (n > anslen) ? anslen : n, stdout);
		}
		goto wait;
	}

	/*
	 * Never leave the socket open.
	 */
	(void) host_res_close(srvsock);

	return (n);
}

/*
** SEND_DGRAM -- Query nameserver via datagram
** -------------------------------------------
**
**	Returns:
**		Length of nameserver answer, if obtained.
**		-1 if an error occurred.
**
**	Inputs:
**		The global variable ``timeout'' should have been
**		set with the desired timeout value in seconds.
**
**	Sending to a nameserver datagram port with no nameserver running
**	will cause an ICMP port unreachable message to be returned. If the
**	socket is connected, we get an ECONNREFUSED error on the next socket
**	operation, and select returns if the error message is received.
**	Also, we get ENETUNREACH or EHOSTUNREACH errors if appropriate.
**	We thus get a proper error status before timing out.
**	This method usually works only if BSD >= 43.
**
**	Note that send() and recvfrom() are now the calls that are allowed
**	to fail under normal circumstances.
*/

static int
send_dgram(addr, query, querylen, answer, anslen)
	input struct sockaddr_in *addr;	/* the server address to connect to */
	input qbuf_t *query;		/* location of formatted query buffer */
	input int querylen;		/* length of query buffer */
	output qbuf_t *answer;		/* location of buffer to store answer */
	input int anslen;		/* maximum size of answer buffer */
{
	char *host = NULL;		/* name of server is unknown */
	HEADER *qp = (HEADER *) query;
	HEADER *bp = (HEADER *) answer;
	register int n;

	/*
	 * Setup a connected (if possible) datagram socket.
	 */
	if ((srvsock = host_res_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		host_res_perror(addr, host, "socket");
		return (-1);
	}
	if (connected) {
		if (connect(srvsock, (struct sockaddr *) addr, sizeof(*addr)) < 0) {
			host_res_perror(addr, host, "connect");
			(void) host_res_close(srvsock);
			return (-1);
		}
	}

	/*
	 * Send the query buffer.
	 */
	if (connected)
		n = send(srvsock, (char *) query, querylen, 0);
	else
		n = sendto(srvsock, (char *) query, querylen, 0,
			   (struct sockaddr *) addr, sizeof(*addr));

	if (n != querylen) {
		if (bitset(RES_DEBUG, _res.options))
			host_res_perror(addr, host, "send");
		(void) host_res_close(srvsock);
		return (-1);
	}

	/*
	 * Wait for the arrival of a reply, timeout, or error message.
	 */
wait:
	if ((n = recv_sock(srvsock, (char *) answer, anslen)) < 0) {
		if (bitset(RES_DEBUG, _res.options))
			host_res_perror(addr, host, "recvfrom");
		(void) host_res_close(srvsock);
		return (-1);
	}

	/*
	 * Make sure it is the proper response by checking the packet id.
	 */
	if (qp->id != bp->id) {
		if (bitset(RES_DEBUG, _res.options))  {
			printf("%sold answer:\n", debug_prefix);
			pr_query(answer, (n > anslen) ? anslen : n, stdout);
		}
		goto wait;
	}

	/*
	 * Make sure it comes from a known server.
	 */
	if (!check_from()) {
		if (bitset(RES_DEBUG, _res.options)) {
			printf("%sunknown server %s:\n",
				debug_prefix, inet_ntoa(from.sin_addr));
			pr_query(answer, (n > anslen) ? anslen : n, stdout);
		}
		goto wait;
	}

	/*
	 * Never leave the socket open.
	 */
	(void) host_res_close(srvsock);

	return (n);
}

#endif /*HOST_RES_SEND*/

/*
** HOST_RES_SOCKET -- Obtain a socket and set additional parameters
** ------------------------------------------------------------
**
**	Returns:
**		socket descriptor if successfully obtained.
**		-1 in case of failure.
**
**	In special circumstances it may be necessary to assign
**	an explicit port number to the client communication socket,
**	e.g. when we are behind a packet filtering firewall that
**	only allows incoming traffic with port numbers in a certain
**	specific range.
**
**	In the case of a stream (tcp) socket, we could have set
**	the SO_REUSEADDR socket option, but this has side-effects.
**	Therefore a single explicit tcp port cannot be used.
**
**	An explicit source IP address may be necessary in case of
**	multi-homed hosts with asymmetric routing policy.
*/

int
host_res_socket(family, type, protocol)
	input int family;
	input int type;
	input int protocol;
{
	struct sockaddr_in res_sin;
	register int port;
	int sock;

	/* try to obtain the desired socket */
	sock = socket(family, type, protocol);
	if (sock < 0)
		return (-1);

	/* set an explicit source address/port if so requested */
	for (port = minport; port > 0 || srcaddr != INADDR_ANY; port++) {
		/* setup source address */
		memset((char *) &res_sin, (int) '\0', sizeof(res_sin));

		res_sin.sin_family = family;
		res_sin.sin_addr.s_addr = srcaddr;
		res_sin.sin_port = htons((u_short)port);

		if (bind(sock, (struct sockaddr *) &res_sin, sizeof(res_sin)) < 0) {
			int save_errno = errno;

			if (port > 0 && errno == EADDRINUSE) {
				/* save_errno = EAGAIN; */
				if (port < maxport)
					continue;
			}

			/* bad source address, or no free port numbers */
			(void) close(sock);
			set_errno(save_errno);
			return (-1);
		}
		if (bitset(RES_DEBUG, _res.options)) {
			if (srcaddr == INADDR_ANY) {
				printf("%susing source port %d\n",
				       debug_prefix, port);
			} else if (port == 0) {
				printf("%susing source address %s\n",
				       debug_prefix, inet_ntoa(res_sin.sin_addr));
			} else {
				printf("%susing source address %s port %d\n",
				       debug_prefix, inet_ntoa(res_sin.sin_addr), port);
			}
		}

		/* socket with well-defined source address/port */
		return (sock);
	}

	/* socket with random source address/port */
	return (sock);
}

/*
** HOST_RES_CLOSE -- close the socket
** --------------------------------------------------------
**
**	Returns:
**		0 if close() was successful.
**		-1 in case of failure.
*/

int
host_res_close(sock)
	input int sock;
{
	return close(sock);
}

/*
** HOST_RES_BLOCKING -- Use blocking or non-blocking socket I/O
** --------------------------------------------------------
**
**	Returns:
**		0 if new mode was successfully set.
**		-1 in case of failure.
**	
**	Under obscure circumstances when reading input from the socket,
**	select() may indicate that data is available, whereas in practice
**	there isn't any, and the subsequent recvfrom() will hang forever.
**	This condition can be detected by reading in non-blocking mode,
**	and restarting the select() if appropriate.
*/

int
host_res_blocking(sock, blocking)
	input int sock;
	input bool_t blocking;		/* indicate blocking or not */
{
	int flags;
	register int n;

#if !defined(WINNT)

	/* fetch current settings */
	n = fcntl(sock, F_GETFL, 0);
	flags = (n == -1) ? 0 : n;

	/* update settings appropriately */
	flags = blocking ? (flags & ~O_NDELAY) : (flags | O_NDELAY);
	n = fcntl(sock, F_SETFL, flags);

#else /*WINNT*/

	/* just set the desired mode */
	flags = blocking ? 0 : 1;
	n = ioctlsocket(sock, FIONBIO, (u_long *) &flags);

#endif /*WINNT*/

	return (n);
}

/*
** HOST_RES_CONNECT -- Connect to a stream (virtual circuit) socket
** ------------------------------------------------------------
**
**	Returns:
**		0 if successfully connected.
**		-1 in case of failure or timeout.
**
**	Note that we use _res.retrans to override the default
**	connect timeout value.
*/

static jmp_buf timer_buf;

/*ARGSUSED*/
sigtype_t
timer(sig)
	int sig;
{
	longjmp(timer_buf, 1);
	/*NOTREACHED*/
}

int
host_res_connect(sock, addr, addrlen)
	input int sock;
	input struct sockaddr_in *addr;		/* the server address to connect to */
	input socklen_t addrlen;
{
	if (setjmp(timer_buf) != 0) {
		set_errno(ETIMEDOUT);
		setalarm(0);
		return (-1);
	}

	setsignal(SIGALRM, timer);
	setalarm(_res.retrans);

	if (connect(sock, (struct sockaddr *) addr, addrlen) < 0) {
		if (errno == EINTR)
			set_errno(ETIMEDOUT);
		setalarm(0);
		return (-1);
	}

	setalarm(0);

	return (0);
}

/*
** HOST_RES_WRITE -- Write the query buffer via a stream socket
** --------------------------------------------------------
**
**	Returns:
**		Length of buffer if successfully transmitted.
**		-1 in case of failure (error message is issued).
**
**	The query is sent in two steps: first a single word with
**	the length of the buffer, followed by the buffer itself.
*/

int
host_res_write(sock, addr, host, buf, bufsize)
	input int sock;			/* socket FD to write to */
	input struct sockaddr_in *addr;	/* the server address to connect to */
	input char *host;		/* name of server to connect to */
	input char *buf;		/* location of formatted query buffer */
	input size_t bufsize;		/* length of query buffer */
{
	u_int len;

	/*
	 * Protect against remote peer prematurely closing the connection.
	 */
	/* setsignal(SIGPIPE, SIG_IGN); done in main() */

	/*
	 * Write the length of the query buffer.
	 */
#if 0
	len = htons((u_short) bufsize);
#else
	ns_put16((u_int) bufsize, (u_char *) &len);
#endif

	if (send(sock, (char *) &len, (sock_buflen_t) INT16SZ, 0) != INT16SZ) {
		host_res_perror(addr, host, "write query length");
		return (-1);
	}

	/*
	 * Write the query buffer itself.
	 */
	if ((size_t) send(sock, buf, (sock_buflen_t) bufsize, 0) != bufsize) {
		host_res_perror(addr, host, "write query");
		return (-1);
	}

	/*
	 * Use non-blocking I/O to read the answer.
	 */
	(void) host_res_blocking(sock, FALSE);

	return (bufsize);
}

/*
** HOST_RES_READ -- Read the answer buffer via a datagram socket
** -------------------------------------------------------
**
**	Returns:
**		Length of (untruncated) answer if successfully received.
**		-1 in case of failure (error message is issued).
**
**	The answer is read in two steps: first a single word which
**	gives the length of the buffer, followed by the buffer itself.
**	If the answer is too long to fit into the supplied buffer,
**	only the portion that fits will be stored, the residu will be
**	flushed, and the truncation flag will be set.
**
**	Note:  The returned length is that of the *un*truncated answer,
**	however, and not the amount of data that is actually available.
**	This may give the caller a hint about new buffer reallocation.
**
**	Note:  This function is currently not used if HOST_RES_SEND is not
**	defined, though all of it's companions are used in list.c.
*/

int
host_res_read(sock, addr, host, buf, bufsize)
	input int sock;			/* socket FD to read from */
	input struct sockaddr_in *addr;	/* the server address to connect to */
	input char *host;		/* name of server to connect to */
	output char *buf;		/* location of buffer to store answer */
	input size_t bufsize;	/* maximum size of answer buffer */
{
	u_short len;
	char *buffer;
	socklen_t buflen;
	size_t reslen;			/* residue length.... */
	register int n;

	/* set stream timeout for recv_sock() */
	timeout = READTIMEOUT;

	/*
	 * Read the length of answer buffer.
	 */
	buffer = (char *) &len;
	buflen = INT16SZ;

	while (buflen > 0 && (n = recv_sock(sock, buffer, buflen)) > 0) {
		buffer += n;
		buflen -= n;
	}
	if (buflen != 0) {
		host_res_perror(addr, host, "recv_sock(): error reading answer length");
		return (-1);
	}

	/*
	 * Terminate if length is zero.
	 */
#if 0
	len = ntohs(len);
#else
	len = ns_get16((u_char *) &len);
#endif
	if (len == 0) {
		errno = EINVAL;		
		host_res_perror(addr, host, "answer has length of zero");
		return (0);
	}

	/*
	 * Check for truncation.
	 * Do not chop the returned length (len) in case of buffer overflow.
	 */
	reslen = 0;
	if (len > bufsize) {
		if (bitset(RES_DEBUG, _res.options)) {
			printf("%sanswer length %u bytes, bufsize only %lu bytes\n",
			       debug_prefix, (unsigned int) len, (unsigned long) bufsize);
		}
		reslen = len - bufsize;
	}

	/*
	 * Read the answer itself.
	 * Don't read more than we can fit in the supplied buffer.
	 */
	buffer = buf;
	buflen = (reslen > 0) ? bufsize : len;

	while (buflen > 0 && (n = recv_sock(sock, buffer, buflen)) > 0) {
		buffer += n;
		buflen -= n;
	}
	if (buflen != 0) {
		host_res_perror(addr, host, "recv_sock(): error reading answer");
		return (-1);
	}

	/*
	 * If we're truncating then discard the residue to keep connection in sync.
	 */
	if (reslen > 0) {
		HEADER *bp = (HEADER *) buf;
		char junkresbuf[PACKETSZ];

		buffer = junkresbuf;
		buflen = (reslen < sizeof(junkresbuf)) ? reslen : sizeof(junkresbuf);

		while (reslen > 0 && (n = recv_sock(sock, buffer, buflen)) > 0) {
			reslen -= n;
			buflen = (reslen < sizeof(junkresbuf)) ? reslen : sizeof(junkresbuf);
		}
		if (reslen != 0) {
			host_res_perror(addr, host, "read residu");
			return (-1);
		}
		if (bitset(RES_DEBUG, _res.options)) {
			printf("%sresponse truncated to %lu bytes\n",
			       debug_prefix, (unsigned long) bufsize);
		}
		/* set truncation flag */
		bp->tc = 1;
	}

	return (len);
}

/*
** HOST_RES_READ_STREAM -- Read the answer buffer via a stream socket
** --------------------------------------------------------------
**
**	Returns:
**		Length of (untruncated) answer if successfully received.
**		-1 in case of failure (error message is issued).
**
*/
int
host_res_read_stream(sock, addr, host, buf, bufsize)
	input int sock;			/* socket FD to read from */
	input struct sockaddr_in *addr;	/* the server address to connect to */
	input char *host;		/* name of server to connect to */
	output char *buf;		/* location of buffer to store answer */
	input size_t bufsize;		/* maximum size of answer buffer */
{
	char *buffer;
	size_t buflen;
	register int n;

	/* set stream timeout for recv_sock() */
	timeout = READTIMEOUT;

	/*
	 * Read more of the answer itself.
	 * Don't read more than we can fit in the supplied buffer.
	 */
	buffer = buf;
	buflen = bufsize;

	while (buflen > 0 && (n = recv_sock(sock, buffer, buflen)) > 0) {
		buffer += n;
		buflen -= n;
	}
	if (buflen != 0) {
		host_res_perror(addr, host, "recv_sock(): error reading answer");
		return (-1);
	}

	return (bufsize);
}

/*
** RECV_SOCK -- Read from stream or datagram socket with timeout
** -------------------------------------------------------------
**
**	Returns:
**		Length of buffer if successfully received.
**		-1 in case of failure or timeout.
**	Inputs:
**		The global variable ``timeout'' should have been
**		set with the desired timeout value in seconds.
**	Outputs:
**		Sets ``from'' to the address of the packet sender.
*/

int
recv_sock(sock, buffer, buflen)
	input int sock;			/* socket FD to read from */
	output char *buffer;		/* current buffer address */
	input size_t buflen;		/* remaining buffer size */
{
	fd_set fds;
	struct timeval wait;
	socklen_t fromlen;
	register int n;

	wait.tv_sec = timeout;
	wait.tv_usec = 0;
rewait:
	/* FD_ZERO(&fds); */
	memset((char *) &fds, (int) '\0', sizeof(fds));
	FD_SET(sock, &fds);

	/* wait for the arrival of data, or timeout */
	n = select(FD_SETSIZE, &fds, (fd_set *) NULL, (fd_set *) NULL, &wait);
	if (n < 0 && errno == EINTR)
		goto rewait;
	if (n == 0)
		set_errno(ETIMEDOUT);
	if (n <= 0)
		return (-1);
reread:
	/* fake an error if nothing was actually read */
	fromlen = sizeof(from);
	n = recvfrom(sock, buffer, (sock_buflen_t) buflen, 0, from_sa, &fromlen);
	if (n < 0 && errno == EINTR)
		goto reread;
	if (n < 0 && errno == EWOULDBLOCK)
		goto rewait;
	if (n == 0)
		set_errno(ECONNRESET);

	return (n);
}

/*
 * Alternative version for systems with broken networking code.
 *
 * The select() system call may fail on the solaris 2.4 platform
 * without appropriate patches. However, these patches are reported
 * to break client NFS.
 *
 * This version uses an alarm() instead of select(). This introduces
 * additional system call overhead.
 * Note that we cannot use non-blocking I/O in this mode.
 */

#ifdef BROKEN_SELECT

static int
recv_sock(sock, buffer, buflen)
	input int sock;			/* socket FD to read from */
	output char *buffer;		/* current buffer address */
	input size_t buflen;		/* remaining buffer size */
{
	int fromlen;
	register int n;

	if (setjmp(timer_buf) != 0) {
		set_errno(ETIMEDOUT);
		setalarm(0);
		return (-1);
	}

	setsignal(SIGALRM, timer);
	setalarm(timeout);
reread:
	/* fake an error if nothing was actually read */
	fromlen = sizeof(from);
	n = recvfrom(sock, buffer, buflen, 0, from_sa, &fromlen);
	if (n < 0 && errno == EINTR)
		goto reread;
	if (n == 0)
		set_errno(ECONNRESET);

	setalarm(0);

	return (n);
}

#endif /*BROKEN_SELECT*/

/*
** HOST_RES_PERROR -- Issue perror message including host info
** -------------------------------------------------------
**
**	Returns:
**		None.
*/

void
host_res_perror(addr, host, message)
	input struct sockaddr_in *addr;	/* the server address to connect to */
	input char *host;		/* name of server to connect to */
	input char *message;		/* perror message string */
{
	int save_errno = errno;		/* preserve state */

	/* prepend server address and name */
	if (addr != NULL)
		(void) fprintf(stderr, "%s ", inet_ntoa(addr->sin_addr));
	if (host != NULL)
		(void) fprintf(stderr, "(%s) ", host);

	/* issue actual message */
	set_errno(save_errno);
	perror(message);

	/* restore state */
	set_errno(save_errno);

	return;
}
