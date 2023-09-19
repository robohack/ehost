#! /bin/sh
#
#ident "@(#)host:$Name:  $:$Id: nscheck.sh,v 1.5 2006-12-20 20:25:42 -0800 woods Exp $"
#
#	nscheck - check NS RRs at every parent NS
#
# A silly little script to pedantically check that every NS for the
# parent zone has the same records for the requested domain(s),
# checking for NS records by default
#
# For now the comparison must be done visually.
# (ToDo: collect and compare each with the next, show only one set)
#
# Using '-a' should also show any glue records the parent nameservers
# may have -- however modern BIND-9 will return delegations in the
# AUTHORITY section and thus '-a' will fail and no glue records will
# ever be returned.
#
# The following is fun:
#
#	nscheck -V -P . version.bind
#
# it's the equivalent of:
#
#	nscheck -t txt -c ch -P . version.bind

# Exit codes from <sysexits.h>
#
EX_OK=0
EX_USAGE=64
EX_UNAVAILABLE=69

# default setup
#
flags="-r"
type="NS"
pzone=""
class=""
rc=""

argv0=$(basename $0)

while getopts a:c:dP:rt:uvVZ OPT; do
	case "$OPT" in
	d|u|v|Z|r)
		flags="-$OPT $flags"
		;;
	a)
		type="ANY"
		;;
	t)
		type="$OPTARG"
		;;
	c)
		class="-c $OPTARG"
		;;
	P)
		pzone="$OPTARG"
		;;
	V)
		type="TXT"
		class="-c CH"
		;;
	\?)
		exit $EX_USAGE
		;;
	esac
done
shift $(($OPTIND - 1))

for domain in $@; do
	case "$domain" in
	*.*)
		;;
	*)
		domain=${domain}.
	esac
	if [ -z "$pzone" ]; then
		pzone=$(host -v -P $domain 2> /dev/null | fgrep 'parent zone:' | tail -1 | awk '{sub(/\)/, "", $NF); print $NF}')
	fi
	case "$pzone" in
	*.*)
		;;
	*)
		pzone=${pzone}.
	esac
	pub_nslist=$(host --canonskip -t NS $pzone | awk '$2 == "NS" {print $NF}')
	for ns in $pub_nslist ; do
		echo ""
		case "$flags" in
		*-v*|*-d*)
			echo "========================================"
			;;
		esac
		echo "====> Checking $ns for $type $class records for '$domain'...."
		host $flags $class -t $type $domain $ns
		rc=${rc:-$?}
	done
done

exit $rc
