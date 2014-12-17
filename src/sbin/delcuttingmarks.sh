#!/bin/sh
#
# Converts a vdr recording to (DVD-compatible) mpeg using replex
# 2008-07-03 by RollerCoaster

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
     delectes VDRs cutting marks from recording

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

# VDR 1.4
if [ -f "$1"/marks.vdr ]; then
	rm -f "$1"/marks.vdr
fi

# VDR 1.7
if [ -f "$1"/marks ]; then
        rm -f "$1"/marks
fi

if [ $? = 0 ]; then
	echo
	secho "Info:"
	secho "Cutting marks successfully deleted."
	return 0
else
	echo
	secho "Info:"
	secho "No cutting marks found or deleting failed."
	return 1
fi
