/*
 *	vers.c -- global version identifier
 */

#ident "@(#)host:$Name:  $:$Id: vers.c,v 1.6 2003-04-05 22:23:39 -0800 woods Exp $"

#if 0
static char Version[] = "@(#)vers.c	e07@nikhef.nl (Eric Wassenaar) 991529";
#endif

#include "host.h"
#include "glob.h"

char *version = "$Name:  $";

#if defined(apollo)
int h_errno = 0;		/* prevent shared library conflicts */
#endif
