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

#ident "@(#)host:$Name:  $:$Id: file.c,v 1.14 2006-12-21 23:49:30 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)file.c	e07@nikhef.nl (Eric Wassenaar) 991529";
#endif

#include "host.h"
#include "glob.h"

#define MAXCACHENAME	(4 + 2 + MAXDNAME)	/* NNNN/Xdomainname */

static char cachefilebuf[MAXCACHENAME + 1];
static char *cachefile = NULL;	/* full name of real cache file */

static char tempcachebuf[MAXCACHENAME + 1];
static char *tempcache = NULL;	/* full name of temporary cache file */

static int cachefd = -1;	/* cache file descriptor */

time_t cachetime = 0;		/* time of last cache modification */

/*
** CACHENAME -- Construct full name of cache file
** ----------------------------------------------
**
**	Returns:
**		Pointer to specified buffer.
**
**	Cache files are organized in a hashed directory structure,
**	with the 4-digit hash value as the name of the particular
**	directory. A single character prefixes the file name.
**	The cache is relative to the current working directory.
*/

char *
cachename(name, buf, prefix)
	input char *name;		/* name of zone to process */
	output char *buf;		/* where to store the file name */
	input int prefix;		/* single char prefix for file name */
{
	register unsigned int hfunc;
	register char *p, *q;
	register char c;

	/*
	 * Construct the name of the hash directory.
	 */
	for (hfunc = 0, p = name; (c = *p) != '\0'; p++) {
		hfunc = ((hfunc << 1) ^ (lowercase((int) c) & 0377)) % HASHSIZE;
	}

	(void) sprintf(buf, "%04u/%c", hfunc, prefix);

	/*
	 * Construct the full name of the cache file.
	 *
	 * Make sure there are only reasonable characters in the file name.
	 * Obviously there should be no further slashes in the name.
	 */
	for (q = &buf[strlen(buf)], p = name; (c = *p) != '\0'; p++) {
		if (!is_alnum((int) c) && !in_string("-._", c))
			c = '?';

		*q++ = lowercase((int) c);
	}
	*q = '\0';

	return (buf);
}

/*
** CACHE_OPEN -- Open cache file for either read or write
** ------------------------------------------------------
**
**	Returns:
**		File descriptor if successfully opened.
**		-1 in case of errors.
**
**	Outputs:
**		Sets ``cachefd'' to the cache file descriptor.
**		Sets ``cachefile'' to the real cache file name.
**		Sets ``tempcache'' to the temp cache name in case
**		a fresh cache must be created, NULL for read only.
*/

int
cache_open(name, create)
	input char *name;		/* name of zone to process */
	input bool_t create;		/* set to create fresh file */
{
	struct stat st;
	register char *p;

	/* construct the name of the associated cache file */
	cachefile = cachename(name, cachefilebuf, 'C');

	/* temporary cache file is not needed when read only */
	tempcache = create ? cachename(name, tempcachebuf, 'T') : NULL;

	/*
	 * If selected for read only, the cache file must just exist.
	 */
	if (!create) {
		if ((cachefd = open(cachefile, O_RDONLY, 0)) < 0) {
			if (errno != ENOENT || debug || verbose)
				cache_perror("Cannot open", cachefile);
			return (-1);
		}

		/* save last modification time for later reference */
		cachetime = (fstat(cachefd, &st) >= 0) ? st.st_mtime : 0;

		return (cachefd);
	}

	/*
	 * If necessary, create the intermediate cache directories.
	 */
	p = strchr(cachefile, '/');
	while (p) {
		if (p > cachefile) {
			*p = '\0';
			if (stat(cachefile, &st) < 0) {
				if (errno != ENOENT) {
					cache_perror("Cannot stat", cachefile);
					return (-1);
				}
				if (mkdir(cachefile, (mode_t) 0755) < 0) {
					if (errno == EEXIST)
						continue;
					cache_perror("Cannot mkdir", cachefile);
					return (-1);
				}
			} else if ((st.st_mode & S_IFMT) != S_IFDIR) {
				set_errno(ENOTDIR);
				cache_perror("Cannot create", cachefile);
				return (-1);
			}
			*p = '/';
		}
		p = strchr(p + 1, '/');
	}

	/*
	 * Create a fresh temporary cache file.  It will be renamed to the real
	 * cache file after processing has been completed successfully.
	 */
	if ((cachefd = open(tempcache, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
		cache_perror("Cannot open", tempcache);
		return (-1);
	}

	/* last modification time unused during cache write */
	cachetime = 0;

	return (cachefd);
}

/*
** CACHE_CLOSE -- Terminate cache file processing
** ----------------------------------------------
**
**	Returns:
**		Status result of close or rename, if appropriate.
**		0 on success, -1 on errors.
*/

int
cache_close(create)
	input bool_t create;		/* set to create fresh file */
{
	int save_errno = errno;		/* preserve state */
	int status = 0;			/* result status */

	/* we must have a valid file */
	if (cachefd >= 0) {
		/* close the cache; this must succeed when writing */
		status = close(cachefd);
		if (status < 0 && tempcache != NULL) {
			cache_perror("Cannot close", tempcache);

			/* preserve existing cache; prevent rename */
			create = FALSE;
		}

		/* move temp cache to real cache if so requested */
		if ((tempcache != NULL) && create) {
			status = rename(tempcache, cachefile);
			if (status < 0)
				cache_perror("Cannot rename", tempcache);
		} else if (tempcache != NULL) {
			if (unlink(tempcache) < 0)
				cache_perror("Cannot unlink", tempcache);
		}
	}

	tempcache = NULL;
	cachefile = NULL;
	cachefd = -1;

	/* restore state */
	set_errno(save_errno);

	return (status);
}

/*
** CACHE_WRITE -- Write an answer buffer to the local disk cache
** -------------------------------------------------------------
**
**	Returns:
**		Length of buffer if successfully written.
**		-1 in case of failure (error message is issued).
**
**	The buffer is written in two steps: first a single word with
**	the length of the buffer, followed by the buffer itself.
**	Anticipate partial writes, probably not strictly necessary.
*/

int
cache_write(buf, bufsize)
	input char *buf;		/* location of raw answer buffer */
	input size_t bufsize;		/* length of answer buffer */
{
	u_int len;
	char *buffer;
	size_t buflen;
	register int n;

	/* we must have a valid file */
	if (cachefd < 0)
		return (0);

	/*
	 * Write the length of the answer buffer.
	 */
	ns_put16((u_int16_t) bufsize, (u_char *) &len);

	buffer = (char *) &len;
	buflen = INT16SZ;

	while (buflen > 0 && (n = write(cachefd, buffer, buflen)) > 0) {
		buffer += n;
		buflen -= n;
	}
	if (buflen != 0) {
		if (errno == 0)
			set_errno(EIO);
		cache_perror("Cannot write answer length", tempcache);
		return (-1);
	}

	/*
	 * Write the answer buffer itself.
	 */
	buffer = buf;
	buflen = bufsize;

	while (buflen > 0 && (n = write(cachefd, buffer, buflen)) > 0) {
		buffer += n;
		buflen -= n;
	}
	if (buflen != 0) {
		if (errno == 0)
			set_errno(EIO);
		cache_perror("Cannot write answer", tempcache);
		return (-1);
	}

	return (bufsize);
}

/*
** CACHE_READ -- Read an answer buffer from the local disk cache
** -------------------------------------------------------------
**
**	Returns:
**		Expected length of (untruncated) answer.
**		-1 in case of failure (error message is issued).
**
*/
int
cache_read_anslen()
{
	char *buffer;
	size_t buflen;
	register int n;
	u_short len;

	buffer = (char *) &len;
	buflen = INT16SZ;
	n = cache_read(buffer, buflen);
	if (n < 0 || n != (int) buflen) {
		cache_perror("cache_read_anslen(): recv_sock(): error reading answer's length", tempcache);
		return (-1);
	}
	len = ns_get16((u_char *) &len);

	return ((int) len);
}

/*
** CACHE_READ -- Read an answer buffer from the local disk cache
** -------------------------------------------------------------
**
**	Returns:
**		Length of (untruncated) answer if successfully read.
**		-1 in case of failure (error message is issued).
**
**	The answer is read in two steps: first a single word which
**	gives the length of the buffer, followed by the buffer itself.
**	If the answer is too long to fit into the supplied buffer,
**	only the portion that fits will be stored, the residu will be
**	flushed, and the truncation flag will be set.
**
**	Note. The returned length is that of the *un*truncated answer,
**	however, and not the amount of data that is actually available.
**	This may give the caller a hint about new buffer reallocation.
*/

int
cache_read(buf, bufsize)
	output char *buf;		/* location of buffer to store answer */
	input size_t bufsize;		/* maximum size of answer buffer */
{
	char *buffer;
	size_t buflen;
	register int n;

	/* we must have a valid, open, cache file */
	if (cachefd < 0)
		return (0);

	/*
	 * Read the length of answer buffer.
	 */
	buffer = buf;
	buflen = bufsize;

	while (buflen > 0 && (n = read(cachefd, buffer, buflen)) > 0) {
		buffer += n;
		buflen -= n;
	}
	if (buflen != 0) {
		if (errno == 0)
			set_errno(EIO);
		cache_perror("Cannot read full answer", cachefile);
		return (-1);
	}

	return (bufsize);
}

/*
** CACHE_PERROR -- Issue perror message including file info
** --------------------------------------------------------
**
**	Returns:
**		None.
*/

void
cache_perror(message, filename)
	input char *message;		/* extra message string */
	input char *filename;		/* file name for perror */
{
	int save_errno = errno;		/* preserve state */

	/* prepend extra message */
	if (message)
		(void) fprintf(stderr, "%s ", message);

	/* issue actual message */
	set_errno(save_errno);
	perror(filename);

	/* restore state */
	set_errno(save_errno);

	return;
}
