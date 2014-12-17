#!/bin/sh

## update stream type from /etc/default/sysconfig 
## to streamdev-server plugin

. /etc/default/globals
. /etc/default/sysconfig

S_TYPE=0 #TS , default

case $STREAM_TYPE in
    Tablet)
            #extern
            S_TYPE=4
            ;;

    *) 
            ;;

esac

#echo "$STREAM_TYPE:$S_TYPE"
svdrpsend.sh plug streamdev-server StreamType $S_TYPE

