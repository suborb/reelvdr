#!/bin/sh
#
# prepares quick shutdown when ReelVDR is going to standby

. /etc/reel/platform/*.inc


REELFPCTL="$REELFPCTL -nowait"

# definitions used in functions
STANDBY_LED=1
REMOTE_LED=2
RECORD_LED=4
POWER_LED=8
ALL_LED=15

NOCLOCK=0
CLOCK=1

#
# Main
#
#set -x
case $1 in
	""|start)
		if [ -f /tmp/vdr.wakeup ]; then
		    wakeup=$(cat /tmp/vdr.wakeup)
		else
		    wakeup=0
		fi

		if [ "$wakeup" = "2147483647" ] || [ "$wakeup" = "0" ] ; then
			TOPTEXT=""
		else
			TOPTEXT="-toptext TIMER"
		fi

		$REELFPCTL -clearlcd
		$REELFPCTL -setclock -brightness 8 -displaymode $CLOCK
		$REELFPCTL -clearled $POWER_LED
		$REELFPCTL -setled $STANDBY_LED
		$FPLEDS standby
		(sleep 2 ; $REELFPCTL $TOPTEXT -displaymode $CLOCK) &
	#TODO: is this required or already done by output plugin?
		$HDCONTROL -v off	# video off
		$HDCONTROL -V 0	# mute
		;;
	end)
		$REELFPCTL -setclock -brightness 8 -displaymode $NOCLOCK
		$REELFPCTL -clearled $(($POWER_LED+$STANDBY_LED))
		$FPLEDS
		;;
	*)
		;;
esac
