#!/bin/sh

#
# MP3_2_MEDIA
# Version 0.2
# Date 06.08.2008
# use mkisofs and cdrecord
#

# include language
. /etc/default/globals

#set -x

if [ -x /usr/bin/wodim ]; then
	CDRECORD="/usr/bin/wodim"
else
	CDRECORD="/usr/bin/cdrecord"
fi

CDRECORDDEV="dev=/dev/dvd"
MKISOFS="/usr/bin/mkisofs"

TEMPDIR="/media/hd/burn_tmp/"
PLAYLIST=$1
LOG_FILE=/tmp/mp3_2_media.log

ISO=mp3_2_media.iso


OLD_IFS=$IFS
IFS='
'
FILE=`cat $PLAYLIST`

exec > $LOG_FILE 2>&1

start() {
#delete old files
rm -f $TEMPDIR/*

#check dir
if [ ! -d $TEMPDIR ]; then
	mkdir -p $TEMPDIR
fi

}

check_cd() {

CHECK_RW=`$CDRECORD -media-info | grep "Mounted media type" |awk '{ print $4 }'`
if [ "$CHECK_RW" = "CD-RW" ]; then
	echo "CD-RW found"
	$CDRECORD -v $CDRECORDDEV blank=fast
elif [ "$CHECK_RW" = "" ]; then
	smesgtl "No media found"
	exit
fi

}

play_list_copy() {

for i in $FILE; do
	cp -f "$i" "$TEMPDIR"
	check_file_size=`du -b $TEMPDIR | tail -1 | awk '{ print $1 }'`
	if [ "$check_file_size" -gt  $((700*1024*1024)) ]; then
		echo "To many files"
		smesgtl "To many files"
		exit
	fi
done

if [ "$?" = "0" ] ; then
	echo "Copy OK"
else
	echo "Copy error"
	exit
fi

}

generate_iso() {

IFS=$OLD_IFS
$MKISOFS -r -J -o $TEMPDIR/$ISO $TEMPDIR

if [ "$?" = "0" ] ; then
	echo "ISO OK"
else
	echo "ISO ERROR"
	exit
fi

}


start_burn() {

$CDRECORD -v $CDRECORDDEV speed=8 $TEMPDIR/$ISO
if [ "$?" = "0" ] ; then
	echo "Burn OK"
else
	echo "Burn ERROR"
	smesgtl "Burn Error"
	exit
fi
	smesgtl "CD burning finished"
	echo DONE
	mount.sh eject
}

# start
(
  start
  check_cd
  play_list_copy
  generate_iso
  start_burn
) 2>&1 &

