#	@(#)Makefile            e07@nikhef.nl (Eric Wassenaar) 991515

#ident "@(#)host:$Name:  $:$Id: Makefile,v 1.9 2003-03-21 19:12:04 -0800 woods Exp $"

# ----------------------------------------------------------------------
# Adapt the installation directories to your local standards.
# ----------------------------------------------------------------------

PREFIX = /usr/local

# This is where the 'host' executable will be referenced.
BINDIR = ${PREFIX}/bin

# This is where manual pages will be referenced.
MANDIR = ${PREFIX}/man/man1

# This is where the rblookup, etc. config file(s) will be referenced.
CONFDIR= ${PREFIX}/etc

# This is where the 'host' executable will be installed.
DESTBIN = ${DESTDIR}/${BINDIR}

# This is where the 'host' manual page will be installed.
DESTMAN = ${DESTDIR}/${MANDIR}

# This is where the config files will be installed.
DESTCONF = ${DESTDIR}/${CONFDIR}

# ----------------------------------------------------------------------
# Special compilation options may be needed only on a few platforms.
# See also the header file port.h for portability issues.
# ----------------------------------------------------------------------

#if defined(_AIX)
SYSDEFS = -D_BSD -D_BSD_INCLUDES -U__STR__ -DBIT_ZERO_ON_LEFT -DHAVE_INET_ATON
#endif

#if defined(SCO) && You have either OpenDeskTop 3 or OpenServer 5
SYSDEFS = -DSYSV
#endif

#if defined(ultrix) && You are using the default ultrix <resolv.h>
SYSDEFS = -DULTRIX_RESOLV
#endif

#if defined(solaris) && You do not want to use BSD compatibility mode
SYSDEFS = -DSYSV -DHAVE_INET_ATON
#endif

#if defined(solaris) && You are using its default broken resolver library
SYSDEFS = -DNO_YP_LOOKUP
#endif

#if defined(NetBSD) && you have not removed support for $HOSTALIASES
SYSDEFS = ${ALLOW_HOSTALIASES}

#if you have a sane, modern, system
SYSDEFS = -DHAVE_INET_ATON

# ----------------------------------------------------------------------
# Configuration definitions.
# See also the header file conf.h for more configuration definitions.
# ----------------------------------------------------------------------

#if defined(BIND_49) && __res_state is still shipped as struct state
CONFIGDEFS = -DOLD_RES_STATE
#endif

#if defined(BIND_48) && You want to use the default bind res_send()
CONFIGDEFS = -DBIND_RES_SEND
#endif

#if defined(BIND_49) && You want to use the special host res_send()
CONFIGDEFS = -DHOST_RES_SEND
#endif

# This is the default in either case if you compile stand-alone.
CONFIGDEFS = -DBIND_RES_SEND
CONFIGDEFS = -DHOST_RES_SEND
CONFIGDEFS = 

# ----------------------------------------------------------------------
# Include file directories.
# This program must be compiled with the same include files that
# were used to build the resolver library you are linking with.
# ----------------------------------------------------------------------

INCL = ../../include
INCL = .
#INCL = /usr/local/bind/include

COMPINCL = ../../compat/include
COMPINCL = .

INCLUDES = -I$(INCL) -I$(COMPINCL)

# ----------------------------------------------------------------------
# Compilation definitions.
# ----------------------------------------------------------------------

DEBUGDEFS= -DDEBUG

DEFS = $(CONFIGDEFS) $(DEBUGDEFS) $(SYSDEFS) $(INCLUDES)

COPTS = -pipe

COPTIM= -O2
COPTIM= -O

CDEBUG= -g

CWARN= -Wall -Wshadow -Wswitch -Wreturn-type -Wpointer-arith -Wconversion -Wimplicit -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes

CFLAGS = $(COPTS) $(CDEBUG) $(COPTIM) $(CWARN) $(DEFS)

# Select your favorite compiler.
CC = /usr/ucb/cc			#if defined(solaris) && BSD
CC = /bin/cc -arch m68k -arch i386	#if defined(next)
CC = /bin/cc -Olimit 1000		#if defined(ultrix)
CC = /bin/cc
CC = cc

# ----------------------------------------------------------------------
# Linking definitions.
# libresolv.a should contain the resolver library of BIND 4.8.2 or later.
# Link it in only if your default library is different.
# SCO keeps its own default resolver library inside libsocket.a
#
# lib44bsd.a contains various utility routines, and comes with BIND 4.9.*
# You may need it if you link with the 4.9.* resolver library.
#
# libnet.a contains the getnet...() getserv...() getproto...() calls.
# It is safe to leave it out and use your default library.
# With BIND 4.9.3 the getnet...() calls are in the resolver library.
# ----------------------------------------------------------------------

RES = -lsocket				#if defined(SCO) && default
RES = ../../res/libresolv.a
RES = -lresolv
RES = -L/usr/local/bind/lib -lbind
RES =					#if BSD

COMPLIB = ../../compat/lib/lib44bsd.a
COMPLIB = -lnet
COMPLIB =

LIBS = -lsocket -lnsl			#if defined(solaris) && not BSD
LIBS =

LIBRARIES = $(RES) $(COMPLIB) $(LIBS)

LDFLAGS = -static

# ----------------------------------------------------------------------
# Miscellaneous definitions.
# ----------------------------------------------------------------------

MAKE = make $(MFLAGS)

# This assumes the BSD install.
INSTALL = install -c

# Grrr
SHELL = /bin/sh

# ----------------------------------------------------------------------
# Files.
# ----------------------------------------------------------------------

PROG =	host
HDRS =	port.h conf.h exit.h type.h rrec.h defs.h host.h glob.h
SRCS =	main.c info.c list.c addr.c geth.c util.c misc.c test.c \
	file.c send.c vers.c
OBJS =	main.o info.o list.o addr.o geth.o util.o misc.o test.o \
	file.o send.o vers.o
MANS =	host.1
DOCS =	RELEASE_NOTES

# the scripts need to be renamed with a ".sh" extension so that their
# generated targets can be created with no extension, ready for
# installation....
#
UTILS = nscheck.sh nslookup.sh mxlookup.sh rblookup.sh
UTIL_PROGS = nscheck nslookup mxlookup rblookup

MISCS = malloc.c README_NT

FILES = Makefile $(DOCS) $(HDRS) $(SRCS) $(MANS) $(UTILS) $(MISCS)

PACKAGE = host
TARFILE = $(PACKAGE).tar

CLEANUP = $(OBJS) $(TARFILE) $(TARFILE).Z

# ----------------------------------------------------------------------
# Rules for installation.
# ----------------------------------------------------------------------

BINOWN = root
BINGRP = staff
BINMODE  = 755
STRIPFLAG = -s

all: $(PROG) $(UTIL_PROGS)

$(OBJS): $(HDRS)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBRARIES)

install: $(PROG)
	$(INSTALL) -m $(BINMODE) -o $(BINOWN) -g $(BINGRP) $(STRIPFLAG) $(PROG) $(DESTBIN)

install-utils: $(UTIL_PROGS)
	$(INSTALL) -m $(BINMODE) -o $(BINOWN) -g $(BINGRP) $(UTIL_PROGS) $(DESTBIN)

install-man: $(MANS)
	$(INSTALL) -m 444 host.1 $(DESTMAN)

clean:
	rm -f $(CLEANUP) *.o a.out core

clobber: clean
	rm -f $(PROG) $(UTIL_PROGS) *.tmp

.SUFFIXES: .tmp

.sh:
	@rm -f $@
	sed -e 's,@BINDIR@,$(BINDIR),g' -e 's,@CONFDIR@,$(CONFDIR),g' < $@.sh > $@
	chmod +x $@

# ----------------------------------------------------------------------
# host may be called with alternative names, querytype names and "zone".
# A few frequently used abbreviations are handy.
# ----------------------------------------------------------------------

ABBREVIATIONS = a ns cname soa wks ptr hinfo mx txt	# standard
ABBREVIATIONS = mb mg mr minfo				# deprecated
ABBREVIATIONS = md mf null gpos				# obsolete
ABBREVIATIONS = rp afsdb x25 isdn rt nsap nsap-ptr	# new
ABBREVIATIONS = sig key px aaaa loc nxt srv kx cert	# very new
ABBREVIATIONS = eid nimloc atma naptr			# draft
ABBREVIATIONS = uinfo uid gid unspec			# nonstandard
ABBREVIATIONS = maila mailb any				# filters

ABBREVIATIONS = mx ns soa zone

links:
	for i in $(ABBREVIATIONS) ; do \
		(cd $(DESTBIN) ; ln -s $(PROG) $$i) ; \
	done

# ----------------------------------------------------------------------
# Rules for maintenance.
# ----------------------------------------------------------------------

lint:
	lint $(DEFS) $(SRCS)

alint:
	alint $(DEFS) $(SRCS)

llint:
	lint $(DEFS) $(SRCS) -lresolv

print:
	lpr -J $(PACKAGE) -p Makefile $(DOCS) $(HDRS) $(SRCS)

dist:
	tar cf $(TARFILE) $(FILES)
	compress $(TARFILE)

# Keep it simple....
#
$(OBJS): $(HDRS)
