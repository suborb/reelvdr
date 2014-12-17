#!/bin/sh

#
# Total restore xxv database
#

PATH="/sbin:/bin/:/usr/bin"


SYSCONFIG="/etc/default/sysconfig"
. $SYSCONFIG


# delete all Tables from xxv
recover() {
    rm /var/lib/mysql/xxv/*
	sleep 1
    /etc/init.d/mysql restart
	sleep 1
    /etc/init.d/xxvd restart
}

recover
