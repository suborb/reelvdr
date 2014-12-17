#!/bin/sh
#
# Converts a vdr recording to (DVD-compatible) mpeg using replex
# 2006-11-23 by RollerCoaster

. /etc/default/globals

AUDIO_FLAGS=""
OUTPUT_DIR="/media/hd/video"
myname=`basename $0`
RECORDINGSDIR="/media/hd/recordings"
loglevel=3

syntax () {
cat << EOF
Usage: $MyName <input directory>
Description:
     delectes DVD archive marks from recording

Options:
     <input directory>	Full path to directory of the recording

EOF
}


#
# Main
#

if [ $# -lt 1 ] ; then
	syntax
	exit 1
fi


if [ ! -d $1 ] ; then
	Syslog "Recording $1 not found"
	syntax
	exit 2
fi


if [ -f "$1"/dvd.vdr ]; then
	rm -f "$1"/dvd.vdr "$1"/index_archive.vdr
fi

if [ $? = 0 ]; then
	secho "Info: Archivemarks successfully deleted."
	return 0
else
	secho "No archivemarks found or deleting failed."
	return 1
fi
