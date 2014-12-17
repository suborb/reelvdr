#!/bin/sh

# ckeck Apple
if [ ! -d /media/hd/music/ipod ]; then
    mkdir -p /media/hd/music/ipod
fi
    
check=`cat /etc/mtab | grep Apple*|awk '{print $1}'`

mount $check /media/hd/music/ipod

