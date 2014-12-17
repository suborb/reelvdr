#!/bin/sh

. /etc/default/sysconfig

### functions
Syslog () {
    logger -t `basename $0` -s "$1"
}


#
# Main
#
if [ "$OPTDIRLINK" = "" ]; then
    OPTDIRLINK="hd"
fi

echo "OPTDIRLINK: $OPTDIRLINK"

MNTPOINTS="ext1 ext2 hd net1 net2 ram"

MNTPOINT=""

if cat /proc/mounts | grep -q /media/$OPTDIRLINK ; then
    # these dirs should always be there
    for i in etc bin lib/vdr ; do mkdir -p /media/$OPTDIRLINK/opt/$i ; done

    if [ -f /var/opt ] && [ ! -L /var/opt ]; then
        mv /var/opt /var/opt.org
    fi
    Syslog "linking /opt -> /var/opt -> /media/$OPTDIRLINK/opt"
    ln -sfn /media/$OPTDIRLINK/opt /var/opt
else
    Syslog "/media/$OPTDIRLINK not mounted"
    MNTPOINT="ram"
fi

if [ "$MNTPOINT" = "ram" ]; then
    if [ -h /var/opt ]; then
	rm -f /var/opt
	mkdir /var/opt
    fi
fi
