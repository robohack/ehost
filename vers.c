#ident "@(#)host:$Name:  $:$Id: vers.c,v 1.3 2003-03-28 21:57:22 -0800 woods Exp $"
#ifndef lint
static char Version[] = "@(#)vers.c	e07@nikhef.nl (Eric Wassenaar) 991529";
#endif

char *version = "host version $Name:  $";

#if defined(apollo)
int h_errno = 0;		/* prevent shared library conflicts */
#endif
