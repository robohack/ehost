#
#ident "@(#)host:$Name:  $:$Id: Makefile,v 1.11 2003-03-31 21:59:02 -0800 woods Exp $"
#
# from:	@(#)Makefile            e07@nikhef.nl (Eric Wassenaar) 991515

# ----------------------------------------------------------------------
# Adapt the installation directories to your local standards.
#
# (It's really sad that Stu Feldman's original Seventh Edition UNIX
# Make implementation didn't have the most useful "?=" and "+="
# assignment operators....)
# ----------------------------------------------------------------------

PREFIX = /usr/local
SHARE = /share

# This is where the 'host' executable will be referenced.
BINDIR = ${PREFIX}/bin

# This is where manual pages will be referenced.
MANDIR = ${PREFIX}${SHARE}/man/man1
CATMANDIR = ${PREFIX}${SHARE}/man/cat1
CATMANSUF = .1

# This is where the rblookup, etc. config file(s) will be referenced.
CONFDIR= ${PREFIX}/etc

# This is where the 'host' executable will be installed.
DESTBIN = ${DESTDIR}/${BINDIR}

# This is where the 'host' manual page will be installed.
DESTMAN = ${DESTDIR}/${MANDIR}

# This is where the 'host' manual page will be installed.
DESTCATMAN = ${DESTDIR}/${CATMANDIR}

# This is where the config files will be installed.
DESTCONF = ${DESTDIR}/${CONFDIR}

# ----------------------------------------------------------------------
# Special compilation options may be needed only on a few platforms.
# See also the header file port.h for portability issues.
# ----------------------------------------------------------------------

#if defined(_AIX)
#SYSDEFS = -D_BSD -D_BSD_INCLUDES -U__STR__ -DBIT_ZERO_ON_LEFT
#endif

#if defined(SCO) && You have either OpenDeskTop 3 or OpenServer 5
#SYSDEFS = -DSYSV
#endif

#if defined(ultrix) && You are using the default ultrix <resolv.h>
#SYSDEFS = -DULTRIX_RESOLV
#endif

#if defined(solaris) && You are using its default broken resolver library
#SYSDEFS = -DNO_YP_LOOKUP
#endif

#if you have a sane, modern, system this should be all you need!
#SYSDEFS = 
#endif

# ----------------------------------------------------------------------
# Configuration definitions.
# See also the header file conf.h for more configuration definitions.
# ----------------------------------------------------------------------

#if defined(BIND_4_9) && __res_state is still shipped as struct state
#CONFIGDEFS = -DOLD_RES_STATE
#endif

#if defined(BIND_4_8) && You want to use the default bind res_send()
#CONFIGDEFS = -DBIND_RES_SEND
#endif

#if defined(BIND_4_9) || newer && You want to use the special host res_send()
#CONFIGDEFS = -DHOST_RES_SEND
#endif

# By default we want to use the latest avaliable resolver API.
#
# Keep in mind that if your resolver library has hooks for using
# non-DNS naming systems (YP, HESIOD, etc.), and if your target
# system(s) might make use of those hooks, then using the system
# resolver will cause grief to "host" users who expect only to query
# the DNS!  Either install a pure BIND resolver library to avoid this,
# or switch to the HOST_RES_SEND option above.
#
CONFIGDEFS = -DBIND_RES_SEND

# ----------------------------------------------------------------------
# Include file directories.
#
# This program _must_ be compiled with the same include files that
# were used to build the resolver library you are linking with.
# ----------------------------------------------------------------------

#if defined(LOCAL_LIBBIND)
#RES_INCL = -I/usr/local/bind/include
#endif
#if defined(SYSTEM_LIBBIND)
#RES_INCL = -I/usr/include/bind
#endif

# compatability headers...
#COMPAT_INCL = -I/usr/local/include/compat

INCLUDES = $(RES_INCL) $(COMPAT_INCL)

# ----------------------------------------------------------------------
# Compilation definitions.
# ----------------------------------------------------------------------

DEBUGDEFS = -DDEBUG

DEFS = $(CONFIGDEFS) $(DEBUGDEFS) $(SYSDEFS) $(INCLUDES)

COPTS = -pipe

COPTIM = -O2
COPTIM = -O

CDEBUG = -g

# GCC lint-like warnings -- any warnings are likely bugs in the
# platform headers or in gcc itself....
#
#if $(__GNUC__) >= 1
GCCWARNFLAGS = -W \
 -Wall \
 -Wimplicit \
 -Wreturn-type \
 -Wswitch \
 -Wcomment \
 -Wcast-qual \
 -Wid-clash-30 \
 -Wpointer-arith \
 -Wshadow
#endif

#if $(__GNUC__) >= 2
GCC2WARNFLAGS = -Waggregate-return \
 -Wcast-align \
 -Wchar-subscripts \
 -Wconversion \
 -Wmissing-declarations \
 -Wmissing-prototypes \
 -Wno-format-extra-args \
 -Wundef \
 -Wlarger-than-65536 \
 -Wbad-function-cast
#endif

#if $(__GNUC__) >= 3
# Yuck:  this is broken in at least 3.2.2...
#GCC3WARNFLAGS = -Wunreachable-code
#endif

CPPFLAGS = $(DEFS)
CFLAGS = $(COPTS) $(CDEBUG) $(COPTIM) $(GCCWARNFLAGS) $(GCC2WARNFLAGS) $(GCC3WARNFLAGS)

# Select your favorite compiler if make doesn't already know it...
#if defined(next)
#CC = cc -arch m68k -arch i386
#else f defined(ultrix)
#CC = cc -Olimit 1000
#else if gcc != cc
#CC = gcc
#endif

# ----------------------------------------------------------------------
# Linking definitions.
#
# WARNING!!!
# WARNING!!! Old resolver libraries have remotely exploitable bugs!
# WARNING!!!
# WARNING!!! DO NOT use older resolver libraries with untrusted data!
# WARNING!!!
#
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
#
# This program _must_ be linked with the resolver library associated
# with the header files you compiled with.
# ----------------------------------------------------------------------

#if defined(SCO) && default
#RES_LIB = -lsocket
#endif
#if defined(NEED_LIBRESOLV) || (sunos5.x)
#RES_LIB = -lresolv
#endif
#if defined(LOCAL_LIBBIND) || (sunos5.x < 5.9)
#RES_LIB = -L/usr/local/bind/lib -lbind
#endif
#if defined(SYSTEM_LIBBIND)
#RES_LIB = -lbind
#endif

#if defined(NEED_LIBNET)
#COMPAT_LIB = -lnet
#endif
#if defined(NEED_LIB44BSD)
#COMPAT_LIB = -l44bsd
#endif

#if defined(sunos5.x)
#SYS_LIBS = -lsocket -lnsl
#endif

LIBRARIES = $(RES_LIB) $(COMPAT_LIB) $(SYS_LIBS)

# host may often be invoked by "root" -- it's safest to static-link it
#
# Unfortunately SunOS-5.9 has only libresolv.so !!!
#
#if defined(NEED_LIBRESOLV) && !defined(sunos5.x)
LDFLAGS = -static
#endif

# ----------------------------------------------------------------------
# Miscellaneous definitions.
# ----------------------------------------------------------------------

#
MAKE = make $(MFLAGS)
SHELL = /bin/sh

# This assumes a BSD-compatible install(1)
INSTALL = install -c

# ----------------------------------------------------------------------
# Files.
# ----------------------------------------------------------------------

PROG =	host
HDRS =	port.h conf.h exit.h type.h rrec.h defs.h host.h glob.h
SRCS =	main.c info.c list.c addr.c geth.c util.c misc.c test.c \
	file.c send.c vers.c
OBJS =	main.o info.o list.o addr.o geth.o util.o misc.o test.o \
	file.o send.o vers.o
MAN =	host.1
MANCAT = host.cat1
DOCS =	RELEASE_NOTES

UTILS = nscheck.sh nslookup.sh mxlookup.sh rblookup.sh
UTIL_PROGS = nscheck nslookup mxlookup rblookup

MISCS = malloc.c README_NT

FILES = Makefile $(DOCS) $(HDRS) $(SRCS) $(MAN) $(UTILS) $(MISCS)

CLEANUP = $(OBJS)

# ----------------------------------------------------------------------
# install options
# ----------------------------------------------------------------------

BINOWN = root
BINGRP = staff
BINMODE = 755
#STRIPFLAG = -s

# ----------------------------------------------------------------------
# basic rules for building and installing a program and its docs
# ----------------------------------------------------------------------

.PHONY: all
all: $(PROG) $(UTIL_PROGS)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBRARIES)

.PHONY: install
install: install-prog install-utils install-man

.PHONY: install-prog
install-prog: $(PROG)
	$(INSTALL) -m $(BINMODE) -o $(BINOWN) -g $(BINGRP) $(STRIPFLAG) $(PROG) $(DESTBIN)

.PHONY: install-utils
install-utils: $(UTIL_PROGS)
	$(INSTALL) -m $(BINMODE) -o $(BINOWN) -g $(BINGRP) $(UTIL_PROGS) $(DESTBIN)

.PHONY: install-man
install-man: $(MAN)
	$(INSTALL) -m 444 $(MAN) $(DESTMAN)

# Note: this target is not automatically depended upon....
.PHONY: install-catman
install-catman: $(MANCAT)
	$(INSTALL) -m 444 $(MANCAT) $(DESTCATMAN)/host$(CATMANSUF)

.PHONY: clean
clean:
	rm -f $(CLEANUP) *.o a.out core

.PHONY: clobber
clobber: clean
	rm -f $(PROG) $(UTIL_PROGS) $(MANCAT) host.0 .depend

# You might need this rule if your default Make rules are too old and
# broken and don't include $(CPPFLAGS)...
#.c.o:
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

.SUFFIXES: .sh

.sh:
	@rm -f $@
	sed -e 's,@BINDIR@,$(BINDIR),g' -e 's,@CONFDIR@,$(CONFDIR),g' < $@.sh > $@
	chmod +x $@

# ----------------------------------------------------------------------
# host may be called with alternative names, querytype names and "zone".
# A few frequently used abbreviations are handy.
# ----------------------------------------------------------------------

#ABBREVIATIONS = a ns cname soa wks ptr hinfo mx txt	# standard
#ABBREVIATIONS = mb mg mr minfo				# deprecated
#ABBREVIATIONS = md mf null gpos 			# obsolete
#ABBREVIATIONS = rp afsdb x25 isdn rt nsap nsap-ptr	# new
#ABBREVIATIONS = sig key px aaaa loc nxt srv kx cert	# very new
#ABBREVIATIONS = eid nimloc atma naptr			# draft
#ABBREVIATIONS = uinfo uid gid unspec			# nonstandard
#ABBREVIATIONS = maila mailb any 			# filters

ABBREVIATIONS = mx ns soa zone

links:
	for i in $(ABBREVIATIONS) ; do \
		(cd $(DESTBIN) ; ln -s $(PROG) $$i) ; \
	done

# ----------------------------------------------------------------------
# Rules for maintenance.
# ----------------------------------------------------------------------

# this is intended for xlint, specifially the version in NetBSD....
#
lint:
	lint -aa -c -h -p -r -s $(DEFS) $(SRCS)

# ----------------------------------------------------------------------
# Dependencies.
# ----------------------------------------------------------------------
# Keep it simple....  it's not that big a program!
#
$(OBJS): $(HDRS)
