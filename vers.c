/*
 *	vers.c -- global version identifier
 */

#ident "@(#)host:$Name:  $:$Id: vers.c,v 1.4 2003-03-28 22:19:57 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)vers.c	e07@nikhef.nl (Eric Wassenaar) 991529";
#endif

char *version = "host version $Name:  $";

#if defined(apollo)
int h_errno = 0;		/* prevent shared library conflicts */
#endif
