#!/bin/ash

#
# (C) 2006 by Reel Multimedia AG http://www.reelbox.org
# Authors: Tobias Bratfisch <tobias@reel-multimedia.com>
# Licence: GPL2
#
# $1 = file
# $2 = type of DVD: "IMAGE" or "DIR"
#
# Date 11.09.2006: Fix DVD Play for DIR
#      29.11.2006: fixed formating disc, cleanups
#

DVD_PROG="growisofs"
DVD_REC_IMG_OPTS=" -use-the-force-luke=tty -dvd-compat -Z"
DVD_REC_DIR_OPTS="-Z"
DVD_REC_DIR_LAST_OPTS="-dvd-video"
DVDWRITER="/dev/dvd"
EJECT="eject -r $DVDWRITER"
LOG="/var/log/dvdswitch_burn.log"
BACKGROUND_OPTS="> $LOG 2>&1 &"

# include language
. /etc/default/globals

    echo 1st $1
    echo 2nd $2

BURN_SPEED=`grep burn.BurnSpeed /etc/vdr/setup.conf | cut -f3 -d" "`
if [ "$BURN_SPEED" = "" ] ; then
	BURN_SPEED=4
fi

#set -x

DVD_REC_IMG_OPTS="-speed=$BURN_SPEED $DVD_REC_IMG_OPTS"
DVDTYPE=`dvd+rw-mediainfo /dev/dvd | grep "Mounted Media:" | cut -b25-27`

case $DVDTYPE in
	14h| DVD-RW)
		DVDBLANK="dvd+rw-format -blank"
		echo "Found DVD-RW"
		;;
	1Ah| DVD+RW)
		# no formating needed
		DVDBLANK=/bin/true
		echo "Found DVD+RW"
		;;
	1Bh| DVD+R)
		# no formating needed
		DVDBLANK=/bin/true
		echo "Found DVD+R"
		;;
	11h| DVD-R)
		# no formating needed
		DVDBLANK=/bin/true
		echo "Found DVD-R"
		;;
	13h| DVD-RWR)
		DVDBLANK="dvd+rw-format -blank"
		echo "Found DVD-RWR"
		;;
	2Bh| DVD+RDL)
		# no formating needed
		DVDBLANK=/bin/true
		echo "Found DVD+RDL"
		;;
	10h| DVD-ROM|*)
		smesgtl "Media is not writeable"
		echo "Found DVD-ROM or unknown media"
		$EJECT
		;;
esac

foreground=false
case $3 in 
	-fg)
		foreground=true
		BACKGROUND_OPTS=""
		#shift
		;;
	*)
		echo unknown option $3
		exit 1
		;;
esac

if [ "$2" = "IMAGE" ]; then
	eval "(
		smesgtl "DVD burning started"
		nice $DVDBLANK $DVDWRITER
		nice $DVD_PROG $DVD_REC_IMG_OPTS $DVDWRITER="$1"
		smesgtl "DVD burning finished"
		echo DONE
		mount.sh eject
	) $BACKGROUND_OPTS "

elif [ "$2" = "DIR" ]; then
	eval "(
		smesgtl "DVD burning started"
		nice $DVDBLANK $DVDWRITER
		echo Doing nice $DVD_PROG $DVD_REC_DIR_OPTS $DVDWRITER $DVD_REC_DIR_LAST_OPTS "$1"
		nice $DVD_PROG $DVD_REC_DIR_OPTS $DVDWRITER $DVD_REC_DIR_LAST_OPTS "$1"
		smesgtl "DVD burning finished"
		echo DONE
		mount.sh eject
	) $BACKGROUND_OPTS "
else
	echo unknown option $2
fi

