#! /bin/sh
#
#	@(#)mxlookup            e07@nikhef.nl (Eric Wassenaar) 950108
#
# Author:	E.Wassenaar, Nikhef-H
# Version:	09-OCT-1994
# Revision:	08-JAN-1995, Make sure servers come from NS records
#
#ident "@(#)host:$Name:  $:$Id: mxlookup.sh,v 1.2 2003-06-04 06:30:27 -0800 woods Exp $"
#
# This utility looks up the MX and A records for a given domain name
# at each of the authoritative servers for the zone it belongs to.
# The printout shows whether or not the servers have the same notion.
# Too often mail is bounced with a "No address or MX record" message
# whereas the mail address is perfectly valid. Usually this happens
# when one of the servers turns out to be misconfigured and the data
# was retrieved unfortunately from just that server.
#
# With the -t option you can verify resource records of another type
# at each of the servers. The -v option shows the SOA record and the
# NS records of the zone to which the given domain name belongs.
# The -r option disables nameserver recursion at the contacted servers.

# ----------------------------------------------------------------------
# Internal vriables.
# ----------------------------------------------------------------------

cmd=`basename $0`

options="[-v] [-r] [-t type]"
usage="Usage: $cmd $options name"

# ----------------------------------------------------------------------
# Exit codes from <sysexits.h>
# ----------------------------------------------------------------------

EX_OK=0
EX_USAGE=64
EX_UNAVAILABLE=69

# ----------------------------------------------------------------------
# Process options.
# ----------------------------------------------------------------------

type=""
recurse=
verbose=
exec=

skip=
for i
do
	if [ $skip ]
	then
		skip=
		continue
	fi

	case "$i" in
	-t)	
		case "$2" in
		""|-*)	echo "$usage" 1>&2 ; exit $EX_USAGE
		esac

		type="$2"
		skip=true
		shift
		;;
	-r)
		recurse="-r"
		;;
	-d)
		exec=echo
		;;
	-v)
		verbose=true
		;;
	-*)
		echo "$cmd: Unknown option $i" 1>&2 ; exit $EX_USAGE
		;;
	*)
		break
		;;
	esac
	shift
done

# ----------------------------------------------------------------------
# Process arguments.
# ----------------------------------------------------------------------

name="$1"

if [ "X$name" = "X" ]
then
	echo "$usage" 1>&2 ; exit $EX_USAGE
fi

# Remove trailing dots.
name=`echo $name | sed 's/\.*$//'`

# ----------------------------------------------------------------------
# Main loop.
# ----------------------------------------------------------------------

domain="$name"

while [ "X$domain" != "X" ]
do
	# Make sure there is at least one dot.
	parentdomain=`echo $domain | sed 's/^[^.]*\.//'`

	if [ "X$parentdomain" = "X$domain" ]
	then
		domain=""
		continue
	fi

	# Find the servers for this domain.
	servers=`host -t NS "$domain" | awk '$2 == "NS" {print $3}'`

	if [ "X$servers" = "X" ]
	then
		# Move to parent domain and retry.
		domain="$parentdomain"
		continue
	fi

	if [ "X$domain" != "X$name" ]
	then
		echo
	fi

	if [ $verbose ]
	then
		echo "--- $domain ---"
		$exec host -T -t SOA "$domain"
		$exec host -T -t NS  "$domain"
		echo
	fi

	for server in $servers
	do
		echo "--- $server ---"
		if [ "X$type" = "X" ]
		then
			$exec host $recurse -T -t MX "$name" "$server"
			$exec host $recurse -T -t A  "$name" "$server"
		else
			$exec host $recurse -T -t "$type" "$name" "$server"
		fi
		echo
	done

	exit $EX_OK
done

exit $EX_UNAVAILABLE
