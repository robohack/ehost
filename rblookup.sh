#! /bin/sh
#
#	@(#)rblookup            e07@nikhef.nl (Eric Wassenaar) 990127
#
# Author:	E.Wassenaar, Nikhef-H
# Version:	11-NOV-1997
# Revision:	02-NOV-1998, Select MAPS (default) or alternatively ORBS
# Revision:	07-NOV-1998, Anticipate hosts with multiple addresses
# Revision:	27-JAN-1999, ORBS has moved to a new site
#
# Ongoing maintenance by Greg A. Woods <woods@planix.com>
#
#ident "@(#)host:$Name:  $:$Id: rblookup.sh,v 1.1 2002-01-11 22:28:47 -0800 woods Exp $"
#
# Lookup a dotted quad IP address, or hostname in one of many
# Reverse/Realtime Blackhole Lists
#
#	Syntax:
#		rblookup hostname ...
#		rblookup dottedquad ...
#		rblookup x.x.x.x.in-addr.arpa ...
#
#	Returns:
#		zero if the given host was found on the blacklist.
#		nonzero if not, or if undetermined.
#
# The RBLs, or sometimes just DNS Black Lists (DNSBLs) are on-line,
# dynamically updated databases of spam hosts, maintained via the DNS.
# The search key is the reversed dotted quad IP address of the given
# host, within the zone of the RBL, for example "rbl.mail-abuse.com".
# A query should be done for an A resource record.  If it exists, the
# given host is blacklisted as a notorious spam host.  The usefulness
# of the value of the retrieved A record depends upon which RBL is
# being queried.  Additional info may often be found via an extra
# query for a TXT resource record.
#
# This script is just an example of a quick and dirty wrapper for the
# ``host'' utility.  The technique can relatively easy be integrated
# into MTA programs like sendmail.
#
# TODO:
#
#	- support name-based DNS Black Lists (eg. rfc-ignorant.org)

# ----------------------------------------------------------------------
# Setup environment.
# ----------------------------------------------------------------------

# This is where the ``host'' executable lives.
#
BINDIR=@DESTBIN@

PATH=${BINDIR}:/bin:/usr/bin ; export PATH

# ----------------------------------------------------------------------
# Internal vriables.
# ----------------------------------------------------------------------

argv0=$(basename $0)

options="[-dv] [-r rbl_list]"
usage="Usage: $argv0 $options hostname|IP# ..."
reversed=""

# ----------------------------------------------------------------------
# RBLs:
# ----------------------------------------------------------------------
#
# A semi-canonical list of RBLs and associated commentary can be found
# at both of the following sites:
#
#	http://www.declude.com/junkmail/support/ip4r.htm
#
#	http://www.iki.fi/era/rbl/rbl.html	# very dated 2001/11/17
#
# some published stats by one user of many DNS Black Lists.
#
#	http://www.sdsc.edu/~jeff/spam/Blacklists_Compared.html

# MAPS RBL: Mail Abuse Protection System Realtime Blackhole List
# <URL:http://mail-abuse.org/rbl/>
# 
# This listing is human-operated, and is one of the most popular
# blacklists of spam-originating systems.  Originally started by Paul
# Vixie, this is the original "RBL" system.  Now a pay-for service.
#
MAPS_ROOT="blackholes.mail-abuse.org"
ALL_RBLS="${MAPS_ROOT} ${ALL_RBLS}"

# MAPS DUL: Mail Abuse Protection System Dial-up User List
# <URL:http://mail-abuse.org/dul/>
# 
# This list was started from a list compiled by "Afterburner" (the
# head-honcho abuse guy at Erols Internet), and extended to be easily
# added to over time.  This service was originally called the ORCA
# DUL, and was operated at "dul.orca.bc.ca".  The MAPS project invited
# Al Iverson, the then-current DUL maintainer, to host it on their
# systems.  Now a pay-for service.
# 
DUL_ROOT="dialups.mail-abuse.org"
ALL_RBLS="${DUL_ROOT} ${ALL_RBLS}"

# MAPS RSS: MAPS Relay Spam Stopper
# <URL:http://mail-abuse.org/rss/>
# 
# This is a list of e-mail relays which have been verified to have
# been used to send spam.  They supposedly have archives of the spam
# they received through these relays.  They do not list multi-level
# relays (and thus miss a vast number of the actively exploited
# systems out there).  Now a pay-for service.
#
RSS_ROOT="relays.mail-abuse.org"
ALL_RBLS="${RSS_ROOT} ${ALL_RBLS}"

# ORBL: Open Relay Black List
# <URL:http://www.orbl.org/>
#
# ORBL is apparently dead as of 2001/10/30....
# 
# ORBL is an auto adjusting spam filter.
# 
# ORBL watches current incoming Internet email traffic for spam
# signatures and adapts to block spam coming from open mail relays.
# 
#ORBL_ROOT="or.orbl.org"
#ALL_RBLS="${ORBL_ROOT} ${ALL_RBLS}"

# ORBZ UK - first son of ORBS
# <URL:http://orbz.gst-group.co.uk/>
#
# Died in Dec. 2001
#
# -returns 127.0.0.2 for outputs
# -returns 127.0.0.3 for inuts
#
#ORBZUK_ROOT="orbz.gst-group.co.uk"
#ALL_RBLS="${ORBZUK_ROOT} ${ALL_RBLS}"

# ORBS NZ - second son of ORBS
# <URL:http://www.orbz.org/>
#
ORBZ_ROOT="inputs.orbz.org outputs.orbz.org"
ALL_RBLS="${ORBZ_ROOT} ${ALL_RBLS}"

# ORDB - third son of ORBS
# <URL:http://www.ordb.org/>
#
ORDB_ROOT="relays.ordb.org "
ALL_RBLS="${ORDB_ROOT} ${ALL_RBLS}"

# DORKSLAYERS - the second edition
# <URL:http://www.dorkslayers.com/>
#
DORKSLAYERS_ROOT="orbs.dorkslayers.com ztl.dorkslayers.com"
ALL_RBLS="${DORKSLAYERS_ROOT} ${ALL_RBLS}"

# Osirusoft Open Relay Spam Stopper
# <URL:http://relays.osirusoft.com/>
#
#   127.0.0.2		Verified Open Relay
#   A verified open relay in most cases is handled by other rbl servers, and
#   imported into the Zone file.
#
#   127.0.0.3		Dialup Spam Source
#   Dialup Spam Sources are imported into the Zone file from other sources and
#   some known sources are manually added to the local include file.
#
#   127.0.0.4		Confirmed Spam Source
#   A site has been identified as a constant source of spam, and is manually
#   added.  Submissions for this type of spam require multiple nominations
#   from multiple sites.
#
#   127.0.0.5		Smart Host (In progress)
#   A Smart host is a site determined to be secure, but relays for those who
#   are not, defeating one level of security.  When this is ready, it will be
#   labeled outputs.osirusoft.com.
#
#   127.0.0.6		A Spamware software developer or spamvertized site.
#   This information will automatically be maintained by www.spamsites.org
#   shortly.  A Spamware software developer.
#
#   127.0.0.7		A list server that automatically subscribes without confirmation
#
#   127.0.0.8		An insecure formmail.cgi script. (Planned)
#
#   127.0.0.9		Open proxy servers
#
#     o Relays.OsiruSoft.com contains all zones, except for outputs and
#       blocktest.  Effectively, it's the master list containing the minimum
#       casualties subzones.
#
#     o Dialups.relays.OsiruSoft.com contains only sources of direct-to-mx
#       spam which are obviously in dynamic IP pools.
#
#     o Spamsites.relays.OsiruSoft.com contains only sites from spamsites.org.
#
#     o Spamhaus.relays.OsiruSoft.com contains only sites from spamhaus.org.
#
#     o Spews.relays.OsiruSoft.com contains only sites from spews.org.
#
#     o Blocktest.relays.osirusoft.com is a stand-alone zone.  It's meant to
#       block testers from testing a site or netblock for many different
#       reasons and has no practical value.  It's not to be interpreted any
#       other way than to prevent test software from testing other sites.
#
#     o Outputs.relays.osirusoft.com will also be a stand-alone zone, and even
#       though it will be created, it should only be used to warn the servers
#       listed.
#
OSIRUSOFT_ROOT="relays.osirusoft.com"
ALL_RBLS="${OSIRUSOFT_ROOT} ${ALL_RBLS}"

# five-ten-sg.com blackholes
# <URL:http://www.five-ten-sg.com/blackhole.php>
#
FIVETENSG_ROOT="blackholes.five-ten-sg.com"
ALL_RBLS="${FIVETENSG_ROOT} ${ALL_RBLS}"

# SpamCop Blocking List
# <URL:http://spamcop.net/bl.shtml>
#
SPAMCOP_ROOT="bl.spamcop.net"
ALL_RBLS="${SPAMCOP_ROOT} ${ALL_RBLS}"

# dev.null.dk Open Relay List
# <URL:http://dev.null.dk/bl.shtml>
#
DEVNULL_ROOT="dev.null.dk"
ALL_RBLS="${DEVNULL_ROOT} ${ALL_RBLS}"

# VISI.com Relay Stop List
# <URL:http://relays.visi.com/>
#
# Only list confirmed open relays that have sent spam, and
# automatically de-list after 90 days.  Apparently they'll gladly
# remove any listing on request too.
#
VISI_ROOT="relays.visi.com"
ALL_RBLS="${VISI_ROOT} ${ALL_RBLS}"

# xbl.selwerd.cx spam List
# <URL:http://selwerd.cx/xbl/>
#
XBL_ROOT="xbl.selwerd.cx"
ALL_RBLS="${XBL_ROOT} ${ALL_RBLS}"

# spambag.org spam List
# <URL:http://www.spambag.org/>
#
# list of spam support service providers and such
#
# They also have a whitelist
#
SPAMBAG_ROOT="blacklist.spambag.org"
ALL_RBLS="${SPAMBAG_ROOT} ${ALL_RBLS}"

# WireHub.nl lists
# <URL:http://basic.wirehub.nl/spamstats.html>
#
#     * Wirehub DynaBlocker (dynamic IP ranges; a lot of                                                              
#       spam comes straight from dial-up users)                                                                       
#
#	<URL:http://basic.wirehub.nl/dynablocker.html>
#
WIREHUB_DYNABLOCK_ROOT="dynablock.wirehub.net"
ALL_RBLS="${WIREHUB_DYNABLOCK_ROOT} ${ALL_RBLS}"
#
#     * Wirehub DNSBL (IPs of persistent spammers, open                                                               
#       relay scanners & abusers, spamvertized websites)                                                              
#
#	<URL:http://basic.wirehub.nl/blackholes.html>
#
WIREHUB_BLACKHOLES_ROOT="blackholes.wirehub.net"
ALL_RBLS="${WIREHUB_BLACKHOLES_ROOT} ${ALL_RBLS}"

# ----------------------------------------------------------------------
# Exit codes from <sysexits.h>, just in case we are called from a mailer
# ----------------------------------------------------------------------

EX_OK=0
EX_USAGE=64
EX_UNAVAILABLE=69

# ----------------------------------------------------------------------
# Auxiliary routines.
# ----------------------------------------------------------------------

fatal ()
{
	message="$*"
	echo "$message" 1>&2
	exit $EX_USAGE
}

# ----------------------------------------------------------------------
# Process options.
# ----------------------------------------------------------------------

verbose=false
rbllist=""

for i in ${1+$@}; do
	case "$i" in
	-d)
		show_or_exec=echo
		;;
	-r)
		rbllist="$2";
		shift
		;;
	-v)
		verbose=true
		;;
	-?)
		fatal "$usage"
		;;
	-*)
		fatal "$argv0: Unknown option $i"
		;;
	*)
		break
		;;
	esac
	params="$params $i"
	shift
done

[ $# -lt 1 ] && fatal "$usage"

# ----------------------------------------------------------------------
# More routines.
# ----------------------------------------------------------------------

invalid ()
{
	fatal "Invalid dotted quad $address: $1"
}

numeric ()
{
	[ "X$1" = "X" ] && invalid

	# Must be numeric.
	value=$(expr $1 + 0) ; [ "X$value" = "X" ] && invalid "$1 not numeric"

	# Must be in range.
	[ "$value" -lt 0 -o "$value" -gt 255 ] && invalid "$1 not in valid octet range"
}

invert ()
{
	reversed=""
	labels=$(echo "$address" | sed -e 's/\./ /g')
	set - $labels
	case "$#" in
	1)
		numeric $1
		reversed="0.0.0.$1"
		;;
	2)
		numeric $1 && numeric $2
		reversed="0.0.$2.$1"
		;;
	3)
		numeric $1 && numeric $2 && numeric $3
		reversed="0.$3.$2.$1"
		;;
	4)
		numeric $1 && numeric $2 && numeric $3 && numeric $4
		reversed="$4.$3.$2.$1"
		;;
	*)
		invalid "$1 has wrong number of dotted values"
		;;
	esac
}

# ----------------------------------------------------------------------
# Main procedure.
# ----------------------------------------------------------------------

exitstat=""

for name in ${1+$@}; do
	# Canonicalise the case and remove any trailing dots.
	name=$(echo $name | tr '[A-Z]' '[a-z]' | sed 's/\.*$//')

	case $name in
	*.in-addr.arpa)
		# Assume this is already a full dotted quad.
		reversed=$(echo $name | sed 's/.in-addr.arpa//')
		;;
	[0-9]*.[0-9]*.[0-9]*.[0-9]*) # an optimisation shortcut....
		address=$name
		invert
		;;
	*)
		# Try to a resolve domain name into dotted quad.
		address=$(host "$name" | awk '$2 == "A" {print $3}')

		if [ -z "$address" ]; then
			# maybe it was a plain address all along?
			address=$name
			invert
		else
			# a recursive call is easier than testing for
			# multiple addresses and restarting the outer
			# loop
			#
			$0 $params $address
			exit $?
		fi
		;;
	esac

	[ -z "$rbllist" ] && rbllist="$ALL_RBLS"

	for rbl in $rbllist; do
		# Construct proper name in map.
		# 
		name="$reversed.$rbl"

		$verbose && echo ""

		# Note we strongly desire to preserve all of the white
		# space in the output of `host', including all of the
		# newlines (except multiple trailing newlines, but
		# there shouldn't be any of those), ant not to use any
		# temporary files.  This means the value must be
		# quoted when it's used too!
		# 
		a_value=$($show_or_exec host -t A $name 2>&1)
		a_found=$?
		[ $a_found -eq $EX_OK ] && exitstat=$EX_UNAVAILABLE
		[ $a_found -eq $EX_OK -o $verbose = "true" ] && echo "${a_value}"

		# don't bother to look for TXT records if we didn't
		# appear to find an A record...
		# 
		txt_value=$([ $a_found -eq $EX_OK ] && $show_or_exec host -t TXT $name 2>&1)
		txt_found=$?
		[ $txt_found -eq $EX_OK -a \( $verbose = "true" -o -n "${txt_value}" \) ] && echo "${txt_value}"
	done
done

exit ${exitstat:-$EX_OK}
