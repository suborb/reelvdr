#!/bin/bash
#
# compress_recording.sh

. /etc/default/globals

TRANSCODE=/usr/sbin/transcode
LOCKFILE=/tmp/compress_recording
OUTLOG=/tmp/compress_recording.log

BPS=$3
ERR=0


case $1 in
	HIGH) BPS=$((BPS*2/3)) ;;
	*)    BPS=$((BPS/2)) ;;
esac

case $2 in
	HD)   BPS=$((BPS/2)) ;;
esac

echo "Convert from $4 to $5" >$OUTLOG
echo "Compression $1" >>$OUTLOG
echo "Resolution $2" >>$OUTLOG
echo "BPS $3 -> $BPS" >>$OUTLOG

NAME=$(awk '$1=="T"{print $2}' "$4"/info.txt | tr ' ' '_') # no spaces
START_TIME=$(date +%s)
DESC="Compressing recording $NAME"
TASK=$(tl "Compressing")

UpdateBgProgress () {
	CTASK=$1
	PERCENT=$2
	[ -z $PERCENT ] && PERCENT=0
	#svdrpsend.sh PLUG bgprocess PROCESS $NAME $START_TIME 0 $DESC "waiting"
	svdrpsend.sh PLUG bgprocess PROCESS $TASK $START_TIME $PERCENT $NAME > /dev/null
}


smesgtl "Starting compression"
UpdateBgProgress starting 0
#svdrpsend.sh PLUG bgprocess PROCESS $NAME $START_TIME 0 $DESC "started" > /dev/null

if [ -f $4/info.vdr ]; then
	IFILES="$4/[0-9][0-9][0-9].vdr"
	INFO=info.vdr
else
	IFILES="$4/[0-9][0-9][0-9][0-9][0-9].ts"
	INFO=info.txt
fi

rm -rf $5
mkdir -p $5
# copy / adjust info.txt
cp $4/$INFO $5/info.txt
sed -i /^H/d $5/info.txt
#fix framerate setting
if grep -q ^F $5/info.txt ; then
    sed -i s/^F.*/"F 25"/ $5/info.txt
fi
#sed -i /^F.*/d $5/info.txt
#sed -i '/^$/d' $5/info.txt
echo "!" >> $5/info.txt

#rm $LOCKFILE.lock
while true; do
	#wait for resources:
	lockfile-create --use-pid --retry 1 $LOCKFILE
	if [ $? -eq 0 ]; then
		#ok, no other compress_recording running...
		if ps ax | grep -v grep | grep $(basename "$TRANSCODE") > /dev/null; then
			#but a transcode is running (streaming)
			lockfile-remove $LOCKFILE
			sleep 1
		else
			#we should have the resources...
			break
		fi
	fi
	#udpate state for user
	UpdateBgProgress "waiting" 0
	#svdrpsend.sh PLUG bgprocess PROCESS $NAME $START_TIME 0 $DESC "waiting"
#TODO: Check for user abort
done

lockfile-touch $LOCKFILE &
BADGER="$!"

COUNT=0
for FILE in $IFILES; do
	COUNT=$((COUNT+1))
done

POS=0
for FILE in $IFILES; do
	#svdrpsend.sh PLUG bgprocess PROCESS $NAME $START_TIME $((POS*80/COUNT+10)) $DESC "running" > /dev/null
	UpdateBgProgress "compressing" $((POS*80/COUNT+10))
	POS=$((POS+1))
#	echo "$FILE -> $5/$(basename "$FILE")" >>$OUTLOG
#	exec cat $FILE|transcode -v $BPS > $5/$(basename "$FILE") 2>>$OUTLOG
#	ERR=$(($ERR+$?))
	echo "$FILE" >>$OUTLOG
	exec cat $FILE|transcode -v $BPS >> $5/00001.ts 2>>$OUTLOG
	ERR=$(($ERR+$?))
done

kill "${BADGER}"
lockfile-remove $LOCKFILE

#svdrpsend.sh PLUG bgprocess PROCESS $NAME $START_TIME 91 $DESC "create index" > /dev/null
UpdateBgProgress "indexing" 91

echo "genindex" >>$OUTLOG
vdr --genindex=$5
ERR=$(($ERR+$?))

#svdrpsend.sh PLUG bgprocess PROCESS $NAME $START_TIME 100 $DESC "completed" > /dev/null
UpdateBgProgress "completed" 100


echo "force update" >>$OUTLOG
touch /media/reel/recordings/.update

echo "DONE" >>$OUTLOG

if [ $ERR -eq 0 ] ; then
	smesgtl "Compression successfull"
else
	smesgtl "Sorry, compression failed"
fi
#svdrpsend.sh PLUG bgprocess PROCESS $NAME $START_TIME 120 $DESC > /dev/null
UpdateBgProgress "done" 120

