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
#ident "@(#)host:$Name:  $:$Id: rblookup.sh,v 1.4 2003-05-17 00:58:43 -0800 woods Exp $"
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
#	- support name-based DNS Black Lists (RHSBL) (eg. rfc-ignorant.org)
#
#	- move all the DNSBLs to a configuration file, perhaps tagging
#	  each as either default or optional, selecting all with a
#	  command line flag, otherwise just query the default entries.

# ----------------------------------------------------------------------
# Setup environment.
# ----------------------------------------------------------------------

# This is where the ``host'' executable lives.
#
BINDIR=@BINDIR@
CONFDIR=@CONFIDR@

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
#	http://relays.osirusoft.com/cgi-bin/rbcheck.cgi	# has a test interface
#
#	http://openrbl.org/stats.htm	# very little info.
#
#	http://www.rbls.org/	# complete, but just a list and a test
#				# interface, no commentary.
#
#	http://www.iki.fi/era/rbl/rbl.html	# very dated 2001/12/02
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
# Died March 18, 2002.  Replaced by DSBL.
#
#ORBZ_ROOT="inputs.orbz.org outputs.orbz.org"
#ALL_RBLS="${ORBZ_ROOT} ${ALL_RBLS}"

# DSBL - Distributed Sender Boycott List
# <URL:http://www.dsbl.org/>
#
DSBL_ROOT="list.dsbl.org unconfirmed.dsbl.org multihop.dsbl.org"
ALL_RBLS="${DSBL_ROOT} ${ALL_RBLS}"

# ORDB - third son of ORBS
# <URL:http://www.ordb.org/>
#
ORDB_ROOT="relays.ordb.org "
ALL_RBLS="${ORDB_ROOT} ${ALL_RBLS}"

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
DNSRBL_ROOT="dun.dnsrbl.net spam.dnsrbl.net"
ALL_RBLS="${DNSRBL_ROOT} ${ALL_RBLS}"

# DORKSLAYERS - the second edition
# <URL:http://www.dorkslayers.com/>
#
DORKSLAYERS_ROOT="relays.dorkslayers.com orbs.dorkslayers.com ztl.dorkslayers.com"
ALL_RBLS="${DORKSLAYERS_ROOT} ${ALL_RBLS}"

# Osirusoft Open Relay Spam Stopper
# <URL:http://relays.osirusoft.com/>
#
#     * 127.0.0.2 Verified Open Relay
#     * 127.0.0.3 Dialup Spam Source
#       Dialup Spam Sources are imported into the Zone file from other sources
#       and some known sources are manually added to the local include file.
#     * 127.0.0.4 Confirmed Spam Source
#       A site has been identified as a constant source of spam, and is
#       manually added. Submissions for this type of spam require multiple
#       nominations from multiple sites. Test Blockers also find themselves in
#       this catagory.
#     * 127.0.0.5 Smart Host (In progress)
#       A Smart host is a site determined to be secure, but relays for those
#       who are not, defeating one level of security. When this is ready, it
#       will be labeled outputs.osirusoft.com. NOTE: I strongly discourage
#       using outputs due to it being way too effective to be useful.
#     * 127.0.0.6 A Spamware software developer or spamvertized site. This
#       information is maintained by spamsites.org and spamhaus.org.
#     * 127.0.0.7 A list server that automatically opts users in without
#       confirmation
#     * 127.0.0.8 An insecure formmail.cgi script. (Planned)
#     * 127.0.0.9 Open proxy servers
#
#     * Relays.OsiruSoft.com contains all zones, except for outputs and
#       blocktest. Effectively, it*s the master list containing the minimum
#       casualties subzones.
#     * Inputs.relays.OsiruSoft.com contains only insecure mail servers.
#     * Dialups.relays.OsiruSoft.com contains only sources of direct-to-mx
#       spam which are obviously in dynamic IP pools.
#     * Spamsites.relays.OsiruSoft.com contains only sites from spamsites.org.
#     * Spamhaus.relays.OsiruSoft.com contains only sites from spamhaus.org.
#     * Spews.relays.OsiruSoft.com contains only sites from spews.org.
#     * Blocktest.relays.osirusoft.com is a stand-alone zone.  It's meant to
#       block testers from testing a site or netblock for many different
#       reasons and has no practical value. It*s not to be interpreted any
#       other way than to prevent test software from testing other sites.
#     * Outputs.relays.osirusoft.com will also be a stand-alone zone, and even
#       though it will be created, it should only be used to warn the servers
#       listed.
#
OSIRUSOFT_ROOT="relays.osirusoft.com"
BLOCKTEST_OSIRUSOFT_ROOT="blocktest.${OSIRUSOFT_ROOT}"
OUTPUTS_OSIRUSOFT_ROOT="outputs.${OSIRUSOFT_ROOT}"
OSIRUSOFT_ALL="${OSIRUSOFT_ROOT} ${BLOCKTEST_OSIRUSOFT_ROOT} ${OUTPUTS_OSIRUSOFT_ROOT}"
ALL_RBLS="${OSIRUSOFT_ALL} ${ALL_RBLS}"

# spam.exsilla.net blackholes
# <URL:http://www.exsilia.net/>
#
# Lists spam sites that we seen by the exsilia.net systems.  Will block
# IP ranges and upstream providers.
#
# 127.0.0.2: Spamming;
# 127.0.0.3: Sends viruses;
# 127.0.0.5: Abuse or postmaster bounces.
#
# Has TXT records.
#
SPAM_EXSILLA_ROOT="spam.exsilia.net"
ALL_RBLS="${SPAM_EXSILLA_ROOT} ${ALL_RBLS}"

# spamsources.fabel.dk blackholes
# <URL:http://www.fabel.dk/relay/>
#
# They only do the most basic relay test with plain envelope addresses.
#
# 127.0.0.2	Lists direct spam sources.
#
SPAMSOURCES_FABEL_ROOT="spamsources.fabel.dk"
ALL_RBLS="${SPAMSOURCES_FABEL_ROOT} ${ALL_RBLS}"

# five-ten-sg.com blackholes
# <URL:http://www.five-ten-sg.com/blackhole.php>
#
# 127.0.0.2	Lists direct spam sources.
# 127.0.0.3	Lists spam sites before they get into DUL
# 127.0.0.4	Lists bulk mailers that don't use confirmed opt-in
# 127.0.0.5	Lists multi-stage open relays
# 127.0.0.6	Lists single-stage open relays
# 127.0.0.7	Lists IP ranges of companies that ignore spam complaints
# 127.0.0.8	Lists servers running vulnerable web scripts that can send spam
# 127.0.0.9	Lists servers with "other issues."
#
FIVETENSG_ROOT="blackholes.five-ten-sg.com"
ALL_RBLS="${FIVETENSG_ROOT} ${ALL_RBLS}"

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

# Spam Filtering @ Monkeys.com
# <URL:http://www.monkeys.com/anti-spam/filtering/formmail.html>
# <URL:http://www.monkeys.com/anti-spam/filtering/proxies.html>
#
MONKEYSFORMMAIL_ROOT="formmail.relays.monkeys.com"
ALL_RBLS="${MONKEYSFORMMAIL_ROOT} ${ALL_RBLS}"
MONKEYSPROXIES_ROOT="proxies.relays.monkeys.com"
ALL_RBLS="${MONKEYSPROXIES_ROOT} ${ALL_RBLS}"

# Not Just Another Black List
# <URL:http://njabl.org/>
#
# 127.0.0.2	lists open relays and known spam sources
# 127.0.0.3	lists dial-up addresses
#
NJABL_ROOT="dnsbl.njabl.org"
ALL_RBLS="${NJABL_ROOT} ${ALL_RBLS}"

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
# Not functional since since 2002/12/16.  They are apparently planning
# to re-write their software and database and start up again sometime
# in the future.
#
#VISI_ROOT="relays.visi.com"
#ALL_RBLS="${VISI_ROOT} ${ALL_RBLS}"

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

# SPAMHAUS
# <URL:http://www.spamhaus.org/sbl/index.lasso>
#
SPAMHAUS_ROOT="sbl.spamhaus.org"
ALL_RBLS="${SPAMHAUS_ROOT} ${ALL_RBLS}"

# SUMMIT
# <URL:http://www.2mbit.com/sbl.php>
#
# Dead as of about May 21, 2002
#
#SUMMIT_ROOT="blackholes.s2mbit.com"
#ALL_RBLS="${SUMMIT_ROOT} ${ALL_RBLS}"

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
