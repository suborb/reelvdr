#!/bin/bash
#
# Reel Multimedia AG 2005
#

if [ -d /etc/default ] ; then
    . /etc/default/sysconfig
else
    . /etc/sysconfig
fi


REELFPCTL=/sbin/reelfpctl
COUNTFILE=/tmp/vdr.records

if [ -e $COUNTFILE ]; then
	count=`cat $COUNTFILE`
else
	echo 0 > $COUNTFILE
	count=0
fi


case "$1" in
	before)
		echo "Before recording $2"
		let count++
		# TODO: only on AVG
		if [ "$AUTO_NOAD" = "yes" ] ; then
			/usr/sbin/noadcall.sh $* >/dev/null 2>&1 &
		fi
		;;
	after)
		echo "After recording $2"
		let count--
		#chmod to world-readable so ftp users can do what they want
		chgrp ftpusers -R `echo "$2" | cut -f1-5 -d"/"` &
		chmod g+w -R `echo "$2" | cut -f1-5 -d"/"` &
		# TODO: only on Lite
		# if [ "$AUTO_NOAD" = "yes" ]; then
		#	/usr/sbin/noadcall.sh $*
		# fi

		#generate preview
		generate_preview.sh "$2"
		;;
	edited)
		echo "Edited recording $2"
		chgrp ftpusers -R `echo "$2" | cut -f1-5 -d"/"` &
		chmod g+w -R `echo "$2" | cut -f1-5 -d"/"` &
		#generate preview
		generate_preview.sh "$2"
		;;
	move)
		# nothing to do
		;;
	*)
		echo "ERROR: unknown state: $1"
		;;
esac

echo $count > $COUNTFILE

if [ $count -eq 0 ]; then
	$REELFPCTL -clearled 4
else
	$REELFPCTL -setled 4
fi
