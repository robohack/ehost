#ident "@(#)host:$Name:  $:$Id: vers.c,v 1.2 2002-01-11 22:15:31 -0800 woods Exp $"
#ifndef lint
static char Version[] = "@(#)vers.c	e07@nikhef.nl (Eric Wassenaar) 991529";
#endif

char *version = "host version 991529-woods.$Name:  $";

#if defined(apollo)
int h_errno = 0;		/* prevent shared library conflicts */
#endif
