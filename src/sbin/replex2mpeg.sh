#!/bin/sh
#
# Converts a vdr recording to (DVD-compatible) mpeg using replex
# 2006-11-23 by RollerCoaster
# 2010-03-29 fixed by MyCo: work around "/" in output filename, rm double "__"

#set -x

. /etc/default/sysconfig
. /etc/default/globals

# check
if [ ! -d /media/reel/video ]; then
    mkdir -p /media/reel/video
fi


AUDIO_FLAGS=""
myname=`basename $0`

OUTPUT_DIR="/media/reel/video"
RECORDINGSDIR="/media/reel/recordings"

loglevel=3
logfile=/var/log/replex2mpeg.log

syntax () {
cat << EOF
Usage: $MyName <input directory>
Description:
     converts VDR recording into DVD-compatible MPEG

Options:
     <input directory>	Full path to directory of the recording

EOF
}

analyse_recording () {
	replex -s $1/001.vdr > $logfile 2>/dev/null
	for i in `grep "MPEG AUDIO" $logfile | cut -f4 -d" "` ; do
		AUDIO_FLAGS="$AUDIO_FLAGS -a $i"
	done
	for i in `grep "AC3 AUDIO" $logfile | cut -f4 -d" "` ; do
		AUDIO_FLAGS="$AUDIO_FLAGS -c $i"
	done

	VIDEO_FLAGS="-v `grep "MPEG VIDEO" $logfile | cut -f4 -d" "`"
	Log 3 "Audio-flags: $AUDIO_FLAGS, Video-flags: $VIDEO_FLAGS"
}

convert2mpeg () {
	ofilename=`grep "^T " $1/info.vdr | cut -b3- | tr / _`
	Subtitle=`grep "^S " $1/info.vdr | cut -b3- | tr / _`
	if [ "$Subtitle" != "" ] ; then
		ofilename="${ofilename}_-_$Subtitle"
	fi
	if [ -z "$ofilename" ] ; then
		ofilename="`echo $1 | sed s?$RECORDINGSDIR/?? | tr / -`"
	fi
	ofilename=`echo $ofilename | tr " " _  | sed "s/____/_/g" | sed "s/___/_/g" | sed "s/__/_/g"`

	ifiles="$1/[0-9][0-9][0-9].vdr"
	cmd="replex -j -t DVD $VIDEO_FLAGS $AUDIO_FLAGS -o $OUTPUT_DIR/$ofilename.mpeg $ifiles"
	Log 1 "calling $cmd"
	secho "Job started"
	echo
	secho "Hint:"
	secho "File is stored in:"
	echo " "
	secho "Main Menu - Films & DVD - Movie Library"
	( eval `$cmd` ) >>$logfile 2>&1 &
}


#
# Main
#

if [ $# -lt 1 ] ; then
	syntax
	exit 1
fi


if [ ! -d $1 ] ; then
	echo "Recording $1 not found"
	syntax
	exit 2
fi


analyse_recording $1
convert2mpeg $1
