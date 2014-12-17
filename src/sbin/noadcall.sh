#!/bin/sh
# this is an example-script for a noad-call with
# different parameters for a call before or after
# a recording is done
# this script should be called from inside vdr via '-r '
# e.g. vdr '-r /usr/local/sbin/noadcall.sh'

#
# 2012-11-21: use markad for HD recordings (RC)
#             get recording title/episode from info.vdr


#TODO:
#  - don't run for VPS recordings?


# globals
if [ -f /etc/reel/globals ] ; then
	. /etc/reel/globals
else
	. /etc/default/globals
fi

. /etc/default/sysconfig

VDR14="false"
MARKAD="false"
# set the noad-binary here
NOAD="noad"

set -x

# wait for the recording to appear
c=0
while ! [ -f $2/info.vdr ] && ! [ -f $2/info.txt ]; do
	c=`echo $((c+1))`
	sleep 1
	[ $c -gt 5 ] && break
done

# check for HD recording
if  grep -q ^H $2/info.* || [ "$FORCE_MARKAD" = "yes" ] ; then
	MARKAD="true"
	Syslog "using markad for HD recording."
fi



if $MARKAD ; then
	NOAD="markad"
	#TODO: check if markad is available
	if ! which markad >/dev/null ; then
	    Syslog "Error: markad is not available, but required. Exit."
	    exit 1
	fi
else
	# set additional args for every call here here
	ADDOPTS="--ac3 --overlap --jumplogo --comments --savelogo"

	# set special args for a call with 'before' here
	# e.g. set a specail statistikfile
	BEFOREOPTS="--statisticfile=/media/reel/recordings/noadonlinestat"
fi


# set the online-mode here
# 1 means online for live-recording only
# 2 means online for every recording
ONLINEMODE="--online=2"


# set special args for a call with 'after' here
# e.g. backup the marks from the online-call before
#      so you can compare the marks and see
#      how the marks vary between online-mode
#      and normal scan (backup-marks are in marks0.vdr)
AFTEROPTS="nice --background --OSD"

TITLE=`grep ^T $2/info.* | cut -b3-`


if [ "$1" = "before" ]; then
	$NOAD $* $ONLINEMODE $ADDOPTS $BEFOREOPTS
else
	#RecName=`echo $2 | cut -f5-6 -d/ | sed s/_/" "/g`
	#echo "Starting NoAd for $RecName."
	$NOAD $* $ADDOPTS $AFTEROPTS
	#Info
	if $VDR14; then
	    echo "\n"
	    tl "Info:"
	    echo ""
	    tl "NoAd started for:"
	    record_title=`echo $* | awk -F / '{print $(NF-1)}' | sed s/_/" "/g`
	    echo " $record_title"
	else
	    msg=`tl "marking commercials in '%s'" | sed s/%s/"$TITLE"/`
	    echo "$msg"
	fi
fi
