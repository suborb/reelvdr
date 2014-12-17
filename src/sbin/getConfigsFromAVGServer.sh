#!/bin/sh

. /etc/default/globals

Usage()
{
    echo "$0 source_server filename"
    echo "\t filename := all|channels.conf|setup.conf|sysconfig|diseqc.conf"
    echo
}

#get Right hand side of '='
# "Ax + By = c" returns 'c'
GetRHS()
{
	echo "$1" | awk -F'=' '{print $2}'
}

#get left hand side of '='
# "Ax + By = c" returns 'Ax + By'
GetLHS()
{
	echo "$1" | awk -F'=' '{print $1}'
}

CopyConfigFile()
{
    case "$1" in
	"channels.conf")
		cp -v  $1 /etc/vdr/
		if [ $? = 0 ]; then
			Syslog "copied $1 -> /etc/vdr"
		else
			Syslog "Error copying $1 -> /etc/vdr"
		fi
		;;
        
    "favourites.conf")
        DEST="/etc/vdr/plugins/reelchannellist/"
        cp -v $1 $DEST
		if [ $? = 0 ]; then
			Syslog "copied $1 -> $DEST"
		else
			Syslog "Error copying $1 -> $DEST"
		fi
        ;;

	"sysconfig")
		#get the timezone line
		LINE=`grep ZONEINFO sysconfig`

		DEFAULT_ZONEINFO="ZONEINFO=\"Europe/Berlin\""

		#if ZONEINFO found, check if it not set to empty string
		if [ -n $LINE ] ; then
			ZONE_STR=`echo $LINE | awk -F'=' '/ZONEINFO/ {print $2}' `

			#empty ZONE_STR
			if [ -z $ZONE_STR ]; then
				LINE=$DEFAULT_ZONEINFO
			fi

		else
			LINE=$DEFAULT_ZONEINFO
		fi

		echo "\n\nGot line $LINE"

		#replace a ZONEINFO line in /etc/default/sysconfig
		cat /etc/default/sysconfig | sed s%ZONEINFO.*%$LINE% > /tmp/sysconfig.tmp

		# if ZONEINFO was not in the original file then add it
		TMPLINE=`grep ZONEINFO /tmp/sysconfig.tmp`

		if [ -z $TMPLINE ]; then
			echo $LINE >> /tmp/sysconfig.tmp
		fi

		ZONEFILE=`GetRHS $LINE | sed s/\"//g`	#remove the quotes
		cp -v  /usr/share/zoneinfo/$ZONEFILE  /etc/localtime && mv  -v /tmp/sysconfig.tmp /etc/default/sysconfig

		;;

	*)
		;;
    esac
}

SERVER_IP=$1
if [ ! -n  "$SERVER_IP" ]; then
    Usage
fi

GetConfigFile()
{
    if [ -n "$1" ]; then
        FILENAME="$1"
    else
        FILENAME="test"
    fi

    Syslog "Downloading $FILENAME from $SERVER_IP to /tmp/configs"
    mkdir -p /tmp/configs
    cd /tmp/configs

    FILE_NOT_FOUND=0
    OUTPUT=`echo "get configs/$FILENAME $FILENAME.tmp" | tftp $SERVER_IP`
    ERROR=`echo "$OUTPUT" | grep "File not found"`


    if [ -n "$ERROR" ]; then
        FILE_NOT_FOUND=1
        Syslog $ERROR
    fi


    if [ $FILE_NOT_FOUND -eq 1 ]; then
        echo "File Not found! Cleaning up";
        rm $FILENAME.tmp
        return  1
    fi


    if [ -s "$FILENAME.tmp" ]; then
        Syslog "Transfer success"
        mv  $FILENAME.tmp $FILENAME
        #Parse
	CopyConfigFile $FILENAME
    else
        Syslog "Error: empty file"
        rm $FILENAME.tmp
        return  2
    fi

    return  0
}

case $2 in # file name

    "all")
        GetConfigFile "channels.conf"
        RET=$?
        GetConfigFile "sysconfig"
        RET=$(($RET + $?))
        GetConfigFile "setup.conf"
        RET=$(($RET + $?))
        GetConfigFile "diseqc.conf"
        RET=$(($RET + $?))
        GetConfigFile "favourites.conf"
        exit $RET
        ;;
    *)
    set -x
        GetConfigFile $2
        exit $?
        ;;
esac

