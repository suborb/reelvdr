#!/bin/sh
#
# Converts a vdr recording to (DVD-compatible) mpeg using replex
# 2006-11-23 by RollerCoaster
# 2010-03-29 fixed by MyCo: work around "/" in output filename, rm double "__"
# 2012-03-04 by reelbus: fix for () in file name

#set -x

. /etc/default/sysconfig
. /etc/default/globals

# check
if [ ! -d /media/reel/video ]; then
    mkdir -p /media/reel/video
fi


AUDIO_FLAGS=""
#myname=`basename $0`

OUTPUT_DIR="/media/reel/video"
RECORDINGSDIR="/media/reel/recordings"

loglevel=3
logfile=/var/log/$MyName.log

RECORDING="$1"
IsNewVdrRec="false"
IsHD="false"

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
    #check if 001.vdr is TS, then it is, likely, HD!
    replex -s $1/001.vdr 2>&1 | head -n6|grep -q "Checking for TS: confirmed" 

    # found "Checking for TS: confirmed" string ?; check return value == 0
    if [ 0 -eq $? ]; then
        IsHD="true" 
        return 
    fi

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
	if $IsNewVdrRec ; then
	    infofile=info.txt
	else
	    infofile=info.vdr
	fi
	ofilename=`grep "^T " $1/$infofile | cut -b3- | tr / _`
	Subtitle=`grep "^S " $1/$infofile | cut -b3- | tr / _`

	if [ "$Subtitle" != "" ] ; then
		ofilename="${ofilename}_-_$Subtitle"
	fi
	if [ -z "$ofilename" ] ; then
		ofilename="`echo $1 | sed s?$RECORDINGSDIR/?? | tr / -`"
	fi
	ofilename=`echo $ofilename | tr " " _  | sed "s/____/_/g" | sed "s/___/_/g" | sed "s/__/_/g"`

	if $IsNewVdrRec ; then
	    ifiles="'$1'/[0-9][0-9][0-9][0-9][0-9].ts"
	    cmd="cat $ifiles > '$OUTPUT_DIR/$ofilename.mpeg'"
	elif $IsHD ; then
	    # old reelvdr HD recording
	    ifiles="'$1'/[0-9][0-9][0-9].vdr"
	    cmd="cat $ifiles > '$OUTPUT_DIR/$ofilename.mpeg'"
	else
	    ifiles="'$1'/[0-9][0-9][0-9].vdr"
	    cmd="replex -j -t MPEG2 $VIDEO_FLAGS $AUDIO_FLAGS -o '$OUTPUT_DIR/$ofilename.mpeg' $ifiles"
	fi


	Log 1 "calling $cmd"
	secho "Job started"
	echo
	secho "Hint:"
	secho "File is stored in:"
	echo " "
	secho "Main Menu - Films & DVD - Movie Library"
	(eval nice -n 20 "$cmd" ) >>$logfile 2>&1 &
}


#
# Main
#

if [ $# -lt 1 ] ; then
	syntax
	exit 1
fi

> $logfile

if [ ! -d "$RECORDING" ] ; then
	echo "Recording $1 not found"
	syntax
	exit 2
fi

if [ -f "$RECORDING"/00001.ts ]; then
    #is a vdr 1.7 recording
    IsNewVdrRec="true"
    convert2mpeg $RECORDING
else
    if grep -q ^H $1/info.vdr ; then
	IsHD="true"
    else
	analyse_recording $RECORDING
    fi
    convert2mpeg $RECORDING
fi
