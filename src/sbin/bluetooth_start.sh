#!/bin/sh
#
# Disable or Enable BLUETOOTH on Systemstart
# Version 0.1
# Date 08.11.2007
#


PATH="/sbin:/bin/:/usr/bin"


SYSCONFIG="/etc/default/sysconfig"
. $SYSCONFIG

if [ "$BLUETOOTH_OFF" = "yes" ]; then
	sed s/"BLUETOOTH_ENABLED=.*"/"BLUETOOTH_ENABLED=0"/ -i /etc/default/bluetooth
else
	sed s/"BLUETOOTH_ENABLED=.*"/"BLUETOOTH_ENABLED=1"/ -i /etc/default/bluetooth
fi

