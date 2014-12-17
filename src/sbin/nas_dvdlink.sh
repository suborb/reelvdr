#!/bin/sh

SYSCONFIG="/etc/default/sysconfig"
. $SYSCONFIG

DVD_DIR=/media/hd/video/dvd

if [ -d /media/smb/$NAS_DVDDIR ] && [ $START_SMBCL = "yes" ]; then
    ln -s /media/smb/$NAS_DVDDIR $DVD_DIR
else
    rm $DVD_DIR/dvd
fi

