#!/bin/bash

PATTERN="08:54"
DEVICE=eth0
FWFILE=/usr/share/reel/netcv/update/netceiver_98D.tgz
FWFILE_TESTING=/usr/share/reel/netcv/update/netceiver_98H.tgz
CONFFILE=/tmp/netceiver_empty.conf



if [ -f $FWFILE_TESTING ] ; then
	FWFILE=$FWFILE_TESTING
	echo "using $FWFILE"
elif [ -f $FWFILE ] ; then
	echo "using $FWFILE"
else
	echo "No usable firmware file. Cancel."
	exit 1
fi


echo '<?xml version="1.0" encoding="UTF-8"?><x/>' > $CONFFILE

NETCVIP=`ping6 -c 5 -I $DEVICE ff02::1|grep $PATTERN|cut -d ' ' -f 4|sort -u|sed -e 's/:$//'`

echo Found NetCeiver at $NETCVIP

if [ x$NETCVIP != x ]; then

    netcvupdate -d $DEVICE -i $NETCVIP -r -X $FWFILE 
    netcvupdate -d $DEVICE -i $NETCVIP -U $CONFFILE
    netcvupdate -d $DEVICE -i $NETCVIP -R
    exit 0
fi

exit 255

