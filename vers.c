#ifndef lint
static char Version[] = "@(#)vers.c	e07@nikhef.nl (Eric Wassenaar) 990701";
#endif

char *version = "990701";

#if defined(apollo)
int h_errno = 0;		/* prevent shared library conflicts */
#endif
