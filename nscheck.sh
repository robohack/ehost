#! /bin/sh
#
#ident "@(#)host:$Name:  $:$Id: nscheck.sh,v 1.2 2003-03-21 02:00:44 -0800 woods Exp $"
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

for i in ${1+$@}; do
	case "$i" in
	-d|-u|-v|-Z|-r)
		flags="$i $flags"
		;;
	-a)
		type="any"
		;;
	-t)
		type="$2"
		shift
		;;
	-P)
		parent="$2"
		shift
		;;
	-*)
		echo "$argv0: Unknown option $i" 1>&2
		exit $EX_USAGE
		;;
	*)
		break
		;;
	esac
	shift
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
