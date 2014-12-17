#!/bin/bash
#
# externremux.sh

. /etc/default/sysconfig

# CONFIG START
  STREAMQUALITY=$STREAM_QUALITY
  RECDIR=/media/hd/recordings/
  TMP=/tmp/externremux-${RANDOM:-$$}
  TRANSCODE=/usr/sbin/transcode
  LOCKFILE=/tmp/compress_recording
# CONFIG END

# DVD abspielen:
# /Extern;STREAMQUALITY:dvd:dvdfile

# Aufzeichnungen abspielen
# /Extern;STREAMQUALITY:rec:dirname to search
# /Extern;STREAMQUALITY:file:filename to search

mkdir -p $TMP

case ${1:0:1} in
     [A-Z]) IFS=: CMD=($1) IFS= ;;
         *) IFS=: CMD=("$STREAMQUALITY" $1) IFS= ;;
esac

case ${CMD[1]} in
     d*) RECORDING="dvd://${CMD[2]}"
         ;;
     r*) REC=$(eval find $RECDIR -name 20*.rec | grep -m 1 -i "${CMD[2]}")
         if [ -d "$REC" ] ; then
             RECORDING="$REC/[0-9][0-9][0-9][0-9][0-9].ts"
         fi
         ;;
     f*) REC=$(eval find $RECDIR | grep -m 1 -i "${CMD[2]}")
         if [ -f "$REC" ] ; then
             RECORDING="$REC"
         fi
         ;;
esac

OUTLOG=$TMP/out.log
OUTLOG=/tmp/out.log

if [ -f  $TRANSCODE ] ; then
	lockfile-create  --use-pid --retry 1 $LOCKFILE
	if [ $? -eq 0 ] ; then
		#no compress job running so we are free to kill all transcode processes (should be from previous streaming jobs)
		killall -9 transcode
	else
		#a compress job is running...
		exit 1
	fi
	case $CMD in
		iPhone) exec transcode -v  600000 -r IPHONE 2>$OUTLOG ;;
		iPad)   exec transcode -v 1200000 -r IPHONE 2>$OUTLOG ;;
		*)        exec transcode 2>$OUTLOG ;;
	esac
	lockfile-remove $LOCKFILE
	exit 0
fi

mkfifo $TMP/out.avi
(trap "rm -rf $TMP" EXIT HUP INT TERM ABRT; cat $TMP/out.avi) &

case $CMD in

     DSL1000) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=100 \
		-oac mp3lame -lameopts preset=15:mode=3 -vf scale=640:-10,harddup \
		-o $TMP/out.avi -- - &>$OUTLOG ;;
     DSL2000) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=128 \
		-oac mp3lame -lameopts preset=15:mode=3 -vf scale=640:-10,harddup \
		-o $TMP/out.avi -- - &>$OUTLOG ;;
     DSL3000) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=250 \
		-oac mp3lame -lameopts preset=15:mode=3 -vf scale=640:-10,harddup \
		-o $TMP/out.avi -- - &>$OUTLOG ;;
     DSL3500) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=300 \
                -oac mp3lame -lameopts preset=15:mode=3 -vf scale=640:-10,harddup \
                -o $TMP/out.avi -- - &>$OUTLOG ;;
     DSL6000) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=350 \
		-oac mp3lame -lameopts preset=15:mode=3 -vf scale=640:-10,harddup \
		-o $TMP/out.avi -- - &>$OUTLOG ;;
     DSL16000) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=500 \
		-oac mp3lame -lameopts preset=15:mode=3 -vf scale=640:-10,harddup \
		-o $TMP/out.avi -- - &>$OUTLOG ;;
       LAN10) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=4096 \
		-oac mp3lame -lameopts preset=standard -vf scale=640:-10,harddup \
		-o $TMP/out.avi -- - &>$OUTLOG ;;
      WLAN11) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=768 \
		-oac mp3lame -lameopts preset=standard -vf scale=640:-10,harddup \
		-o $TMP/out.avi -- - &>$OUTLOG ;;
      WLAN54) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=2048 \
		-oac mp3lame -lameopts preset=standard -vf scale=640:-10,harddup \
		-o $TMP/out.avi -- - &>$OUTLOG ;;
        UMTS) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=150 \
                -oac mp3lame -lameopts preset=15:mode=3 -vf scale=640:-10,harddup \
                -o $TMP/out.avi -- - &>$OUTLOG ;;
	IPAQ) exec mencoder $RECORDING -ovc lavc -lavcopts vcodec=mpeg4:vbitrate=350 \
		-oac mp3lame -lameopts preset=15:mode=3 -vf scale=640:-10,harddup \
                -o $TMP/out.avi -- - &>$OUTLOG ;;
           *) touch $TMP/out.avi ;;
esac
