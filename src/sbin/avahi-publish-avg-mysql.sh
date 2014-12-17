#!/bin/bash
RETVALUE=0
PIDFILE=/var/run/avahi-publish-avg-mysql.pid

if [ -e $PIDFILE ];
then
	PID=`cat $PIDFILE`
	start-stop-daemon --stop -p $PIDFILE
	rm $PIDFILE
else
	start-stop-daemon --stop -x /usr/bin/avahi-publish-service
fi

if kill -0 $PID 2> /dev/null;
then
#	echo "sleeping..."
	busybox usleep 2000;
fi

if [ $1 -ne -2 ];
then
	if [ $RETVALUE -eq 0 ];
	then
		avahi-publish-service $HOSTNAME _reelboxMySQL._tcp 667 lasteventid=$1 >/dev/null 2>&1 &
		PID=$!
		if kill -0 $PID 2> /dev/null;
		then
			echo $! > $PIDFILE
		else
			RETVALUE=1
		fi
	fi
fi

exit $RETVALUE
