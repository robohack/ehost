/*
 *	vers.c -- global version identifier
 */

#ident "@(#)host:$Name:  $:$Id: vers.c,v 1.5 2003-03-29 02:53:04 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)vers.c	e07@nikhef.nl (Eric Wassenaar) 991529";
#endif

char *version = "$Name:  $";

#if defined(apollo)
int h_errno = 0;		/* prevent shared library conflicts */
#endif
