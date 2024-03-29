#! /bin/sh
#
#	@(#)nslookup            e07@nikhef.nl (Eric Wassenaar) 940919
#
# Author:	E.Wassenaar, Nikhef-H
# Version:	19-SEP-1994
# Revision:
#
#ident "@(#)host:$Name:  $:$Id: nslookup.sh,v 1.2 2003-06-04 06:30:36 -0800 woods Exp $"
#
# This utility emulates nslookup as a wrapper to host.
# It performs most, but not all, functions.

exec=echo
exec=

cmd=`basename $0`

options="[-l]"
usage="Usage: $cmd $options [server]"

# ----------------------------------------------------------------------
# Exit codes from <sysexits.h>
# ----------------------------------------------------------------------

EX_OK=0
EX_USAGE=64
EX_UNAVAILABLE=69

# ----------------------------------------------------------------------
# Setup defaults.
# ----------------------------------------------------------------------

name=""
zone=""

type=""
class=""

server=""

debug=""
verbose=""
norecurse=""
vc=""

# ----------------------------------------------------------------------
# Process arguments.
# ----------------------------------------------------------------------

for i
do
	if [ "X$server" != "X" ]
	then
		echo "$usage" 1>&2 ; exit $EX_USAGE
	fi

	case "$i" in
	-d)	exec=echo		;;
	-l)	server=`hostname`	;;
	-*)	echo "$cmd: Unknown option $i" 1>&2 ; exit $EX_USAGE ;;
	*)	server="$i"		;;
	esac
done

# ----------------------------------------------------------------------
# Choose between BSD or SYSV echo command.
# ----------------------------------------------------------------------

n=`echo -n`
if [ "$n" = "-n" ]
then
	c='\c' ; n=''
else
	n='-n' ; c=''
fi

# ----------------------------------------------------------------------
# Main loop.
# ----------------------------------------------------------------------

trap continue 2

while echo $n "> $c" ; read line
do
	case "$line" in
	"")	continue	;;
	*\**)	continue	;;
	*\!*)	continue	;;
	*\?*)	line="help"	;;
	esac

	set - $line
	command="$1"
	case "$command" in

	help)
		cat <<!
	exit | quit	- exit from the program
	help | ?	- print this help message
	set  | set all	- show all current settings
	set [no]debug	- increase debug output level
	set [no]verbose	- increase verbose output level
	set [no]recurse	- request recursive nameserver queries
	set [no]vc	- use virtual circuit for queries
	set type=TYPE	- query about the given resource record TYPE
	set class=CLASS	- query about the given resource record CLASS
	server SERVER	- contact the explicit server with name SERVER
	server		- show the name of the current explicit SERVER
	NAME		- query for NAME using all current settings
	NAME SERVER	- query for NAME at the given SERVER
	ls ZONE		- generate listing of the given name ZONE
	ls		- generate listing of the last given ZONE
	ls ZONE [>]FILE	- copy resource record output also to FILE
!
		;;

	exit|quit)
		break
		;;

	set)
		option="$2"
		case "$option" in

		""|all)
			echo "name=$name"
			echo "zone=$zone"
			echo "server=$server"
			echo "set type=$type"
			echo "set class=$class"
			echo "set debug=$debug"
			echo "set verbose=$verbose"
			echo "set norecurse=$norecurse"
			echo "set vc=$vc"
			;;

		debug|debu|deb|de|d)
			debug="-d $debug"
			;;

		nodebug|nodebu|nodeb|node|nod)
			debug=""
			;;

		d2)
			debug="-d -d"
			verbose="-v -v"
			;;

		nod2)
			debug=""
			verbose=""
			;;

		verbose|verbos|verbo|verb|ver|ve|v)
			verbose="-v $verbose"
			;;

		noverbose|noverbos|noverbo|noverb|nover|nove|nov)
			verbose=""
			;;

		recurse|recurs|recur|recu|rec|re|r)
			norecurse=""
			;;

		norecurse|norecurs|norecur|norecu|norec|nore|nor)
			norecurse="-r"
			;;

		vc)
			vc="-u"
			;;

		novc)
			vc=""
			;;

		querytype=*|querytyp=*|queryty=*|queryt=*|\
		query=*|quer=*|que=*|qu=*|q=*|\
		type=*|typ=*|ty=*|t=*)
			type=`echo $option | sed 's/.*=//'`
			if [ "X$type" != "X" ]
			then
				type="-t $type"
			fi
			;;

		querytype|querytyp|queryty|queryt|\
		query|quer|que|qu|q|\
		type|typ|ty|t)
			type=""
			;;

		class=*|clas=*|cla=*|cl=*|c=*)
			class=`echo $option | sed 's/.*=//'`
			if [ "X$class" != "X" ]
			then
				class="-c $class"
			fi
			;;

		class*|clas*|cla*|cl*|c*)
			class=""
			;;

		*)
			echo "Unknown option $option"
			;;

		esac
		;;

	server)
		nserver="$2"
		if [ "X$nserver" = "X" ]
		then
			if [ "X$server" = "X" ]
			then
				echo "No server defined"
			else
				echo "server=$server"
			fi
		else
			server="$nserver"
		fi
		;;

	ls)
		nzone="$2"
		if [ "X$nzone" = "X" ]
		then
			if [ "X$zone" = "X" ]
			then
				echo "No zone defined"
				continue
			fi
		else
			zone="$nzone"
		fi

		file=`echo "$3$4" | sed 's/>*//'`
		if [ "X$file" != "X" ]
		then
			file="-f $file"
		fi

		options="$debug $verbose $norecurse $vc $file"
		$exec host $options $type $class -l $zone $nserver
		;;

	[a-zA-Z0-9]*)
		name="$1"

		nserver="$2"
		if [ "X$nserver" = "X" ]
		then
			nserver="$server"
		fi

		options="$debug $verbose $norecurse $vc"
		$exec host $options $type $class $name $nserver
		;;

	*)
		echo "Unknown command $command"
		;;

	esac
done

exit $EX_OK
