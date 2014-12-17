#!/bin/sh
#
# is called by reelbox-control
# kills vdr if it hasn't terminated after 20 sec

#set -x
if [ "$1" = "cancel" ]; then
    (
	sleep 1
	killall `basename $0`
    ) >/dev/null 2>&1 &
else
	(
		sleep 6
		if [ ! -f /tmp/vdr.shutdown ] ; then
			killall -SIGTERM vdr
		fi

		sleep 20
		#if [ ! -f /tmp/vdr.shutdown  ] && [ ! -f /tmp/vdr.standby ] ; then
		if [ ! -f /tmp/vdr.standby ] ; then
			touch /tmp/vdr.standby
			killall -SIGKILL vdr
		fi
	) >/dev/null 2>&1 &
fi
