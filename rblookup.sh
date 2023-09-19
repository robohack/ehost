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
# See RFC-5782 <URL:https://www.rfc-editor.org/rfc/rfc5782>
#
# This script is just an example of a quick and dirty wrapper for the
# ``host'' utility.  The technique it uses to check addresses can also
# be relatively easily integrated into MTA programs like Smail, Exim,
# Postrix, Sendmail, and so on.
#
# TODO:
#
#	- support name-based DNS Black Lists (RHSBL)
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
# at the following sites:
#
#	https://en.wikipedia.org/wiki/Comparison_of_DNS_blacklists
#
#	https://multirbl.valli.org/lookup/		# needs JavaScript
#
#	https://whatismyipaddress.com/blacklist-check	# needs JavaScript
#
#	https://www.rbls.org/	# complete, but just an on-line test interface,
#				# no commentary.
#
#	https://www.rtsak.com/dns-lookup/	# does many things, including
#						# the rbls.org DNSBL lookups
#

# The LashBack "unsubscribe" abuser list:
# <URL:https://blacklist.lashback.com/>
#
# no TXT RRs, not testable (i.e. not RFC-5782 compliant).
#
UNSUB_ROOT="ubl.unsubscore.com"
ALL_RBLS="${ALL_RBLS} ${UNSUB_ROOT}"

# DSBL - Distributed Sender Boycott List
# <URL:http://www.dsbl.org/>
#
# Replaced by Passive Spam Block List
# <URL:https://psbl.org/>
#
PSBL_ROOT="psbl.surriel.com"
ALL_RBLS="${ALL_RBLS} ${PSBL_ROOT}"

# ABUSEAT lists
# The CBL - Composite Blocking List
# <URL:https://www.abuseat.org/>
# <URL:https://cbl.abuseat.org/>
#
# The CBL operates in an entirely automated way using very large
# spamtraps and smart heuristics to detect real spam sources. There
# is an automated no-questions-asked removals procedure too.
#
ABUSEAT_ROOT="cbl.abuseat.org"
ALL_RBLS="${ALL_RBLS} ${ABUSEAT_ROOT}"

# SpamCop Blocking List
# <URL:https://spamcop.net/bl.shtml>
#
SPAMCOP_ROOT="bl.spamcop.net"
ALL_RBLS="${ALL_RBLS} ${SPAMCOP_ROOT}"

# Spamhaus Block List
# <URL:http://www.spamhaus.org/zen/>
#
# ZEN contains all of the SBL, XBL, and PBL
#
SPAMHAUS_ZEN_ROOT="zen.spamhaus.org"
ALL_RBLS="${ALL_RBLS} ${SPAMHAUS_ZEN_ROOT}"

# 0Spam Project
# <URL:https://www.0spam.org/>
#
ZEROSPAM_ROOT="bl.0spam.org"
ALL_RBLS="${ALL_RBLS} ${ZEROSPAM_ROOT}"

# JustSpam
# <URL:http://www.justspam.org/>
#
JUSTSPAM_ROOT="dnsbl.justspam.org"
ALL_RBLS="${ALL_RBLS} ${JUSTSPAM_ROOT}"

# SpamRats
# <URL:https://www.spamrats.com/>
#
SPAMRATS_NOPTR_ROOT="noptr.spamrats.com"
SPAMRATS_DYNA_ROOT="dyna.spamrats.com"
SPAMRATS_SPAM_ROOT="spam.spamrats.com"
SPAMRATS_AUTH_ROOT="auth.spamrats.com"
SPAMRATS_ALL_ROOT="all.spamrats.com"
ALL_RBLS="${ALL_RBLS} ${SPAMRATS_ALL_ROOT}"

# The NordSpam Project
# <URL:https://www.nordspam.com/>
#
# also have a domain blacklist (and a whitelist)
#
NORDSPAM_ROOT="bl.nordspam.com"
ALL_RBLS="${ALL_RBLS} ${NORDSPAM_ROOT}"

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
ALL_RBLS="${ALL_RBLS} ${SORBS_ROOT}"

# spamsources.fabel.dk blackholes
# <URL:http://www.fabel.dk/relay/>
#
# They only do the most basic relay test with plain envelope addresses.
#
# 127.0.0.2	Lists direct spam sources.
#
SPAMSOURCES_FABEL_ROOT="spamsources.fabel.dk"
ALL_RBLS="${ALL_RBLS} ${SPAMSOURCES_FABEL_ROOT}"

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
ALL_RBLS="${ALL_RBLS} ${SPAMGUARD_ROOT}"

# ANTISPAM-UFRJ
# <URL:http://www.aupads.org/>
#
# no TXT RRs, not testable (i.e. not RFC-5782 compliant).
#
# RSBL suspended for now.....
#UFRJ_RSBL_ROOT="rsbl.aupads.org"
#ALL_RBLS="${ALL_RBLS} ${UFRJ_RSBL_ROOT}"
# DUINV discontinued!
#DUINV_RSBL_ROOT="duinv.aupads.org"
#ALL_RBLS="${ALL_RBLS} ${DUINV_RSBL_ROOT}"
ORVEDB_RSBL_ROOT="orvedb.aupads.org"
ALL_RBLS="${ALL_RBLS} ${ORVEDB_RSBL_ROOT}"

# Mailspike - Anubisnetworks reputation and Zero-Hour blocklists
# <URL:https://www.mailspike.net/>
#
# "bl" is the combined reputation ("rep") and zero-hour ("z")
#
# They also have a "whitelist", wl.mailspike.net
#
MAILSPIKE_ROOT="bl.mailspike.net"
ALL_RBLS="${ALL_RBLS} ${MAILSPIKE_ROOT}"

# Good, Bad, Ugly database
# <URL:http://gbudb.com/>
#
GBUDB_ROOT="truncate.gbudb.net"
ALL_RBLS="${ALL_RBLS} ${GBUDB_ROOT}"

# Weighted Private Block List
# <URL:http://www.wpbl.info/>
#
WPBL_ROOT="db.wpbl.info"
ALL_RBLS="${ALL_RBLS} ${WPBL_ROOT}"

# Metunet Research Labs
# <URL:>
#
METUNET_ROOT="rbl.metunet.com"
ALL_RBLS="${ALL_RBLS} ${METUNET_ROOT}"

# Blocklist.de
# <URL:http://www.blocklist.de/en/api.html#dns>
#
BLOCKLIST_DE_ROOT="bl.blocklist.de"
ALL_RBLS="${ALL_RBLS} ${BLOCKLIST_DE_ROOT}"

# NiX Spam
# <URL:https://www.nixspam.net/>
#
NIXSPAM_ROOT="ix.dnsbl.manitu.net"
ALL_RBLS="${ALL_RBLS} ${NIXSPAM_ROOT}"

# s5h.net
# <URL:http://www.usenix.org.uk/content/rbl.html>
#
S5H_ROOT="all.s5h.net"
ALL_RBLS="${ALL_RBLS} ${S5H_ROOT}"

# BarracudaCentral
# <URL:http://www.barracudacentral.org/rbl>
#
# Requires registration of recursive NS resolver source address(es)
#
# no TXT RRs, not testable (i.e. not RFC-5782 compliant).
#
BARRACUDA_ROOT="bl.barracudacentral.org"
ALL_RBLS="${ALL_RBLS} ${BARRACUDA_ROOT}"

# The Abusive Hosts Blocking List
# <URL:https://www.ahbl.org/>
#
# N.B.:  Invite only!  The old zone now blacklists everything, and the TXT says:
#
#	"List shut down.  See: http://www.ahbl.org/content/last-notice-wildcarding-services-jan-1st"
#
#AHBL_ALL_ROOT="dnsbl.ahbl.org"
#ALL_RBLS="${ALL_RBLS} ${AHBL_ALL_ROOT}"


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
