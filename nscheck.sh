#! /bin/sh
#
#ident "@(#)host:$Name:  $:$Id: nscheck.sh,v 1.1 2002-01-11 22:29:38 -0800 woods Exp $"
#
#	nscheck - check NS RRs at every parent NS
#
# A silly little script to pedantically check that every NS for the
# parent zone has the same records for the requested domain(s),
# checking for NS records by default
#
# For now the comparison must be done visually.

# Exit codes from <sysexits.h>
#
EX_OK=0
EX_USAGE=64
EX_UNAVAILABLE=69

# default setup
#
flags="-r"
type="ns"
rc=""

for i
do
	case "$i" in
	-d|-u|-v)
		flags="$i $flags"
		shift
		;;
	-a)
		type="any"
		shift
		;;
	-t)
		type="$2"
		shift 2
		;;
	-P)
		parent="$2"
		shift 2
		;;
	-*)
		echo "$argv0: Unknown option $i" 1>&2
		exit $EX_USAGE
		;;
	*)
		break
		;;
	esac
done

for domain
do
	if [ -z "$parent" ] ; then
		parent=$(echo $domain | sed 's/[^\.]*.//')
	fi
	case "$parent" in
	*.*)
		;;
	*)
		parent="${parent}."
	esac
	nslist=$(host -t ns $parent | awk '{print $NF}')
	for ns in $nslist ; do
		echo ""
		case "$flags" in
		*-v*|*-d*)
			echo "========================================"
			;;
		esac
		echo "====> Checking $ns for $type records for $domain...."
		host $flags -t $type $domain $ns
		rc=${rc:-$?}
	done
done

exit $rc
