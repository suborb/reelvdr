#!/bin/sh
#
# (C) 2006 by Reel Multimedia AG http://www.reelbox.org
# Authors: Tobias Bratfisch <tobias@reel-multimedia.com>
# Licence: GPL2
#
# This scripts gets this parameter
# $1 = Path to image destination
# $2 = New image-name with extension
# $3 = The DVD-Device
# $4 = fstab MountPoint for the DVD-Device
# $5 = Determines if the image will be saved as "IMAGE" or "DIR"
#

IPATH="/media/reel/video/dvd"
INAME=$2
DVDDEV=/dev/dvd
#cracy mountpath on ubuntu...
DVDMP=/media/dvd

# include language
. /etc/default/globals

# Trim spaces of the new Image-name
void=`echo $INAME | sed -e 's/^ */_/' -e 's/ *$//'`

DVDREADER_IMAGE=/bin/dd
DVDREADER_IMAGE_ARGS=" if=$DVDDEV of=\"$IPATH/$INAME\" "
DVDREADER_DIR=/usr/bin/vobcopy
DVDREADER_DIR_ARGS="-o $IPATH -f -q -m -i $DVDMP -L /tmp"

if [ $# -lt 5 ]
then
    echo "ERROR"
    echo "  usage: dvdselect_readdvd.sh targetDirectory imagename DVDdevice mountPoint [IMAGE|DIR]"
    exit 1
fi

rm -f /tmp/vobcopy_1.1.0.log

if [ "$5" = "IMAGE" ]; then
	[ -f $IPATH/$INAME.iso] && rm -f $IPATH/$INAME.iso
	echo "Starting copy... "
	smesgtl "started reading DVD"  2>/dev/null 1>/dev/null
	(
	        echo executing $DVDREADER_IMAGE $DVDREADER_IMAGE_ARGS
		/usr/bin/nice -n 20 $DVDREADER_IMAGE $DVDREADER_IMAGE_ARGS &&
			smesg "DVD $INAME" 2>/dev/null 1>/dev/null
		echo "Unmounting..."
		mount.sh unmount
	) &
elif [ "$5" = "DIR" ]; then

        # mount first vobcopy needs to get DVDName
	if ! `cat /proc/mounts | grep $DVDDEV`; then
		echo "Mounting..."
		mount.sh mount
	fi

        ############## get a UNIQUE dvd name #####################
	#FIXME: directory should be removed if it exists but we need the name first
	#DvdName=`vobcopy -I | grep DVD-name`
	DVDNAME=`/usr/bin/vobcopy  -I 2>&1  | grep DVD-name | cut -d':' -f2| sed 's/^ *//;s/ *$//'`
	# get DVDName and remove first character which is a space, leftover from cut

#	[ -n "$DvdName" ] && [ -d $IPATH/"$DvdName" ] && rm -rf $IPATH/"$DvdName" && echo -e "\033[1m  rm -rf $IPATH/\"$DvdName\"  \033[0m"
	# check for non-empty string ; check if DIR exists ; remove it ; display message

	#if empty DvdName
	[ -z "$DVDNAME" ] && DVDNAME="Unknown DVD"

	#get a new name : stdout, '-' have special meanings
	[ "$DVDNAME" = "stdout" ] && DVDNAME="stdoutDVD"
	[ "$DVDNAME" = "-" ]      && DVDNAME="Unknown DVD"

	tmpDVDNAME=$DVDNAME
	#get a name that doesnot exist in the path $IPATH
	while [ -e $IPATH/"$tmpDVDNAME" ]  			# donot check for dirs using -d
	do ## for ever ??
		sleep 1; # needed else 'date' might not change
        	tmpDVDNAME=$DVDNAME-$(date '+%F-(%H:%M:%S)')
	done

        DVDNAME=$tmpDVDNAME

	mkdir -p "$IPATH"


	if [ $? -eq 0 ] ; then
		(
			echo "Starting copy... "
			smesgtl "started reading DVD" >/dev/null 2>&1
			echo -e "executing \033[1m $DVDREADER_DIR $DVDREADER_DIR_ARGS --name \"$DVDNAME\"\033[0m"
			/usr/bin/nice -n 20 $DVDREADER_DIR $DVDREADER_DIR_ARGS --name "$DVDNAME" &&
				smesgtl "DVD reading done" >/dev/null 2>&1
			echo "Unmounting... "
			#DONE should be in a newline
			echo -e "\nDONE" >> /tmp/vobcopy_1.1.0.log
			mount.sh unmount
			mount.sh eject
		) &
	else
		#DONE should be in a newline
		echo -e "\nDONE" >> /tmp/vobcopy_1.1.0.log  # so bgprocess ends anyway
		smesgtl "DVD not ready" 2>&1 >/dev/null
		mount.sh eject
	fi
fi
