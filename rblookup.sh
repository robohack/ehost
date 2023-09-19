#! /bin/sh
#
#ident "@(#)host:$Name:  $:$Id: rblookup.sh,v 1.13 2008-10-05 21:04:54 -0800 woods Exp $"
#
# rblookup - Lookup a dotted quad IP address, or hostname in one of many
#		Reverse/Realtime DNS-based Lists
#
# Author:	E.Wassenaar, Nikhef-H
#
# Originally from:
#	@(#)rblookup            e07@nikhef.nl (Eric Wassenaar) 990127
#
# Ongoing maintenance by Greg A. Woods <woods@planix.com>
#
# Usage:
#
#	rblookup [-dsv] [-r rbl_list] hostname|IP#
#
#		-d 		- for debugging
#		-r rbl_list	- use the specified RBLs
#		-s		- use Smail's current list of RBLs
#		-v		- verbose output (show each RBL)
#
#	Operand Syntax:
#		rblookup hostname ...
#		rblookup dottedquad ...
#		rblookup x.x.x.x.in-addr.arpa ...
#
#	Returns:
#		zero if the given host was found on the blacklist.
#		nonzero if not, or if undetermined.
#
# The RBLs, or DNS Black/White Lists (DNS[BW]Ls) are on-line,
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
# ``host'' utility.  The technique it uses to check addresses can also
# be relatively easily integrated into MTA programs like Smail, Exim,
# Postrix, Sendmail, and so on.
#
# TODO:
#
#	- support name-based DNS Black Lists (RHSBL) (eg. rfc-ignorant.org)
#
#	- move all the DNSBLs to a configuration file, perhaps tagging
#	  each as either default or optional, selecting all with a
#	  command line flag, otherwise just query the default entries.

# ----------------------------------------------------------------------
# Internal vriables.
# ----------------------------------------------------------------------

argv0=$(basename $0)

options="[-sdv] [-r rbl_list]"
usage="Usage: $argv0 $options hostname|IP# ..."
reversed=""

# ----------------------------------------------------------------------
# RBLs:
# ----------------------------------------------------------------------
#
# A semi-canonical list of RBLs and associated commentary can be found
# at both of the following sites:
#
#	https://whatismyipaddress.com/blacklist-check
#
#	http://www.rbls.org/	# complete, but just an on-line test interface,
#				# no commentary.
#
#	https://www.rtsak.com/dns-lookup/	# does many things, including
#						# the rbls.org DNSBL lookups
#
#	http://www.DNSstuff.com/	# much more than just blacklist testing!
#
# some published stats by some users of DNS Black Lists.
#
#	http://www.sdsc.edu/~jeff/spam/Blacklists_Compared.html
#	http://mail.vene.ws/cgi-bin/stats.pl?blacklist
#	http://abuse.easynet.nl/spamstats.html
#	http://www.dnsbl.net.au/stats/
#

##smtp_rbl_domains = "\
##        :zen.spamhaus.org;127.0.0.2, 127.0.0.4, 127.0.0.5, 127.0.0.6, 127.0.0.7,127.0.0.8, 127.0.0.10, 127.0.0.11\
##        :psbl.surriel.com;127/8\
##        :cbl.abuseat.org;127/8\
##        :dnsbl.sorbs.net;127.0.0.2, 127.0.0.3, 127.0.0.4, 127.0.0.5, 127.0.0.6,127.0.0.7, 127.0.0.8, 127.0.0.9, 127.0.0.10, 127.0.0.11,127.0.0.12\
##        :spamsources.fabel.dk;127.0.0.2\
##        :bl.spamcop.net;127.0.0.2\


# The Abusive Hosts Blocking List
# <URL:http://www.ahbl.org/>
#
# N.B.:  This zone now blacklists everything, and the TXT says:
#
#	"List shut down.  See: http://www.ahbl.org/content/last-notice-wildcarding-services-jan-1st"
#
#AHBL_ALL_ROOT="dnsbl.ahbl.org"
#ALL_RBLS="${AHBL_ALL_ROOT} ${ALL_RBLS}"

# Blitzed Open Proxy Monitor List
# <URL:http://opm.blitzed.org/info>
#
#   WinGate      127.1.0.1
#   SOCKS        127.1.0.2
#   HTTP CONNECT 127.1.0.4
#   Router       127.1.0.8
#   HTTP POST    127.1.0.16
#
BLITZED_ALL_ROOT="opm.blitzed.org"
ALL_RBLS="${BLITZED_ALL_ROOT} ${ALL_RBLS}"

# dev.null.dk Open Relay List
# <URL:http://dev.null.dk/bl.shtml>
#
DEVNULL_ROOT="dev.null.dk"
ALL_RBLS="${DEVNULL_ROOT} ${ALL_RBLS}"

# DNSRBL - 
# <URL:http://www.dnsrbl.com/>
#
# 127.0.0.2	verified open relay
# 127.0.0.3	dialup spam source
# 127.0.0.4	confirmed spam source
# 127.0.0.5	smart host
# 127.0.0.6	a spamware software developer or spamvertized site
# 127.0.0.7	a list server that opts in without confirmation
# 127.0.0.8	an insecure formmail.cgi script
# 127.0.0.9	open proxy servers
#
# GONE?!?!?!?
#
#DNSRBL_ROOT="dun.dnsrbl.net spam.dnsrbl.net"
#ALL_RBLS="${DNSRBL_ROOT} ${ALL_RBLS}"

# DSBL - Distributed Sender Boycott List
# <URL:http://www.dsbl.org/>
#
DSBL_ROOT="list.dsbl.org unconfirmed.dsbl.org multihop.dsbl.org"
ALL_RBLS="${DSBL_ROOT} ${ALL_RBLS}"

# ABUSEAT lists
# The CBL - Composite Blocking List
# <URL:http://www.abuseat.org/>
# <URL:http://cbl.abuseat.org/>
#
# The CBL operates in an entirely automated way using very large
# spamtraps and smart heuristics to detect real spam sources. There
# is an automated no-questions-asked removals procedure too.
#
ABUSEAT_RSBL_ROOT="cbl.abuseat.org"
ALL_RBLS="${ABUSEAT_RSBL_ROOT} ${ALL_RBLS}"

# McFadden Associates Spam Blacklist
# <URL:http://bl.csma.biz/>
#
# N.B.:  Dead and gone.
#
#CSMA_BL_ROOT="bl.csma.biz"
#ALL_RBLS="${CSMA_BL_ROOT} ${ALL_RBLS}"
#CSMA_SBL_ROOT="sbl.csma.biz"
#ALL_RBLS="${CSMA_SBL_ROOT} ${ALL_RBLS}"

# five-ten-sg.com blackholes
# <URL:http://www.five-ten-sg.com/blackhole.php>
#
# N.B.:  Dead and gone.
#
# 127.0.0.2	Lists direct spam sources (by netblock for the misc.spam group).
# 127.0.0.3	Lists spam sites before they get into DUL
# 127.0.0.4	Lists bulk mailers that don't use confirmed opt-in
# 127.0.0.5	Lists multi-stage open relays
# 127.0.0.6	Lists single-stage open relays
# 127.0.0.7	Lists IP ranges of companies that ignore spam complaints
# 127.0.0.8	Lists servers running vulnerable web scripts that can send spam
# 127.0.0.9	Lists servers with "other issues."
#
#FIVETENSG_ROOT="blackholes.five-ten-sg.com"
#ALL_RBLS="${FIVETENSG_ROOT} ${ALL_RBLS}"

# N.B.:  mail-abuse.org has been taken over by trendmicro.com!
#
# MAPS RBL: Mail Abuse Protection System Realtime Blackhole List
# <URL:http://mail-abuse.org/rbl/>
# 
# This listing is human-operated, and is one of the most popular
# blacklists of spam-originating systems.  Originally started by Paul
# Vixie, this is the original "RBL" system.  Now a pay-for service.
#
#MAPS_BLACKHOLES_ROOT="blackholes.mail-abuse.org"
#ALL_RBLS="${MAPS_ROOT} ${ALL_RBLS}"

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
#MAPS_DUL_ROOT="dialups.mail-abuse.org"
#ALL_RBLS="${DUL_ROOT} ${ALL_RBLS}"

# MAPS RSS: MAPS Relay Spam Stopper
# <URL:http://mail-abuse.org/rss/>
# 
# This is a list of e-mail relays which have been verified to have
# been used to send spam.  They supposedly have archives of the spam
# they received through these relays.  They do not list multi-level
# relays (and thus miss a vast number of the actively exploited
# systems out there).  Now a pay-for service.
#
#MAPS_RSS_ROOT="relays.mail-abuse.org"
#ALL_RBLS="${RSS_ROOT} ${ALL_RBLS}"

# Not Just Another Black List
# <URL:http://njabl.org/>
# <URL:http://njabl.org/dynablock.html> (gone -- use pbl.spamhaus.org)
#
# N.B.:  Dead and gone.
#
# 127.0.0.2	lists open relays and known spam sources
# 127.0.0.3	lists dial-up addresses
# 127.0.0.4	lists spam sources
# 127.0.0.5	lists multi-stage open relays
# 127.0.0.8	lists servers with insecure formmail scripts
# 127.0.0.9	lists open proxy sources
#
#NJABL_ROOT="dnsbl.njabl.org"
#ALL_RBLS="${NJABL_ROOT} ${ALL_RBLS}"

# ORDB - third son of ORBS
# <URL:http://www.ordb.org/>
#
# ORDB is shutting down as of 2006/12/18
#
#ORDB_ROOT="relays.ordb.org "
#ALL_RBLS="${ORDB_ROOT} ${ALL_RBLS}"

# 
# <URL:http://dnsbl.net.au/>
#
# (formerly the reynolds boycott list)
#
# Note this is strictly a pay-for subscription service and must not be
# used for production without a subscription.
#
# ``comprises everything of "type 1", which is everything''
#
DNSBL_NET_AU_ROOT="t1.dnsbl.net.au"
ALL_RBLS="${REYNOLDS_T1_BL_ROOT} ${ALL_RBLS}"

# rfc-ignorant IP-based whois List
# <URL:http://www.rfc-ignorant.org/policy-ipwhois.php>
#
# list of nets without proper WHOIS information
#
RFC_IGNORANT_IPWHOIS_ROOT="ipwhois.rfc-ignorant.org"
ALL_RBLS="${RFC_IGNORANT_IPWHOIS_ROOT} ${ALL_RBLS}"

# spambag.org spam List
# <URL:http://www.spambag.org/>
#
# list of spam support service providers and such
#
# They also had a whitelist
#
# N.B.:  Dead and gone.
#
#SPAMBAG_ROOT="blacklist.spambag.org"
#ALL_RBLS="${SPAMBAG_ROOT} ${ALL_RBLS}"

# SpamCop Blocking List
# <URL:http://spamcop.net/bl.shtml>
#
SPAMCOP_ROOT="bl.spamcop.net"
ALL_RBLS="${SPAMCOP_ROOT} ${ALL_RBLS}"

# SpamGuard by Howard Leadmon
# <URL:http://www.leadmon.net/spamguard/>
#
# 127.0.0.2	Dial-Up IP Addresses
# 127.0.0.3	Individual SPAM Sources
# 127.0.0.4	Bulk mailers
# 127.0.0.5	Single-Stage Open Relays
# 127.0.0.6	Multi-Stage Open Relays
# 127.0.0.7	SpamBlock Sites (have sent us direct SPAM)
#
# 127.0.0.1 will always return a negative (not found) result, and can
# be used to verify no response. As such a 127.0.0.2 will return a
# positive on the dialup section, and 127.0.0.3 will return a positive
# on the spam section.  And so on....  (except for '7' 2002/03/21)
#
SPAMGUARD_ROOT="spamguard.leadmon.net"
ALL_RBLS="${SPAMGUARD_ROOT} ${ALL_RBLS}"

# Spamhaus Block List
# <URL:http://www.spamhaus.org/zen/>
#
# ZEN contains all of the SBL, XBL, and PBL
#
SPAMHAUS_ZEN_ROOT="zen.spamhaus.org"
ALL_RBLS="${SPAMHAUS_ZEN_ROOT} ${ALL_RBLS}"

# spamsources.fabel.dk blackholes
# <URL:http://www.fabel.dk/relay/>
#
# They only do the most basic relay test with plain envelope addresses.
#
# 127.0.0.2	Lists direct spam sources.
#
SPAMSOURCES_FABEL_ROOT="spamsources.fabel.dk"
ALL_RBLS="${SPAMSOURCES_FABEL_ROOT} ${ALL_RBLS}"

# SORBS: Spam and Open Relay Blocking System
# <URL:http://www.dnsbl.sorbs.net/>
#
# 127.0.0.2	- open HTTP proxies
# 127.0.0.3	- open SOCKS proxies
# 127.0.0.4	- misc proxies
# 127.0.0.5	- open relays
# 127.0.0.6	- sent spam to sorbs.net
# 127.0.0.7	- vulnerable web servers
# 127.0.0.8	- sites demanding never to be tested
# 127.0.0.9	- hijacked networks
# 127.0.0.10	- Dynamic IP Address ranges
# 127.0.0.11	- domain names with A or MX pointing to bad address space.
# 127.0.0.12	- domains that are never used for e-mail
#
SORBS_ROOT="dnsbl.sorbs.net"
ALL_RBLS="${SORBS_ROOT} ${ALL_RBLS}"

# VISI.com Relay Stop List
# <URL:http://relays.visi.com/>
#
# Only list confirmed open relays that have sent spam, and
# automatically de-list after 90 days.  Apparently they'll gladly
# remove any listing on request too.
#
VISI_ROOT="relays.visi.com"
ALL_RBLS="${VISI_ROOT} ${ALL_RBLS}"

# ANTISPAM-UFRJ
# <URL:http://www.aupads.org/>
#
# no TXT RRs, not testable.
#
UFRJ_RSBL_ROOT="rsbl.aupads.org"
ALL_RBLS="${UFRJ_RSBL_ROOT} ${ALL_RBLS}"
ORVEDB_RSBL_ROOT="orvedb.aupads.org"
ALL_RBLS="${ORVEDB_RSBL_ROOT} ${ALL_RBLS}"
DUINV_RSBL_ROOT="duinv.aupads.org"
ALL_RBLS="${DUINV_RSBL_ROOT} ${ALL_RBLS}"

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
params=""

while getopts dMr:v OPT; do
	case "$OPT" in
	d)
		show_or_exec=echo
		;;
	r)
		rbllist="$OPTARG";
		;;
	s)
		rbllist=$(smail -bP smtp_rbl_domains | sed -e 's/;[^:]*//g;s/:/ /g')
		;;
	v)
		verbose=true
		;;
	\?)
		fatal "$usage"
		;;
	esac
	params="$params -$OPT $OPTARG"
done
shift $(($OPTIND - 1))

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

for name in $@; do
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
