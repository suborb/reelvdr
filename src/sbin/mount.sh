#!/bin/sh
#
# This script is called from VDR to mount/unmount/eject
# the sources for MP3 play.
#
# argument 1: wanted action, one of mount,unmount,eject,status
# argument 2: mountpoint to act on
#
# mount,unmount,eject must return 0 if succeeded, 1 if failed
# status must return 0 if device is mounted, 1 if not
#

. /etc/default/globals

Syslog "$*"

CDOPEN=/tmp/cdopen
mountpoint=/media/dvd
SPEED=12
timeout=120


if RbLite ; then
	# -p: use /proc/mounts instead of /etc/mtab
	EJECT="eject -p"
	SLOTIN=false
	FPLEDS=/etc/init.d/fpleds
else
	# -r: eject ATAPI CD-ROM (also unmounts it)
	# -s: eject SCSI CD-ROM (also unmounts it)
	EJECT="eject -r -s"
	SLOTIN=true
	mountpoint_ipod=/media/Apple_*
	FPLEDS=/bin/true
fi


#set -x

action="$1"
#path="$2"
#test -z "$path" && path="/dev/dvd"
DEVICE=/dev/dvd
DEVICE_IPOD=/media/Apple_*

case "$action" in
	mount)
		#eject -t -p "$DEVICE"                # close the tray
		$FPLEDS  1>&2
		rm $CDOPEN

		#determine device file

		#DEVICE=`grep $DEVICE /etc/fstab | cut -f1 -d" "`
		#if [ "$DEVICE" = "" ]; then
		#	Syslog "Warning: $DEVICE not found in /etc/fstab"
		#fi


		#determine fs type as we only want iso9660 or ufs
		id=`blkid $DEVICE`
		success=$?
		set -- $id
		#sometime DMA access gets resetted by kernel
		hdparm -d1 $DEVICE

		if [ $success = 0 ]; then
			[ -d $mountpoint ] || mkdir $mountpoint
			shift
			eval $*
			if mount -t $TYPE -o ro "$DEVICE" $mountpoint ; then
				Syslog "Successfully mounted $DEVICE"
			fi
		else
			Syslog "$DEVICE does not carry a filesystem. Maybe an audio-cd?"
			ret=1
			break
		fi
		hdparm -d1 $DEVICE
		speedcontrol -x$SPEED "$DEVICE" &        # this is just for setting the drive speed
		ret=0
		;;
	umount|unmount)
		umount -l "$DEVICE"                     # unmount it
		ret=$?
		;;
	eject)
		now=`date +%s`
		if [ -f $CDOPEN ]  && ! $SLOTIN ; then
			last_access=`cat $CDOPEN`
			rm $CDOPEN
			if [ $now -gt $(($last_access+$timeout)) ]; then
				#after $timeout we assume that user has closed tray manually
				$0 eject
			else
				eject -t -p "$DEVICE"  # close the tray
				ret=$?
				speedcontrol -x$SPEED > /dev/null
				$FPLEDS 1>&2 &
			fi
		else
			# brighten FP LEDs
			$FPLEDS trayopen 1>&2 &
			$EJECT "$DEVICE"          # eject disk
			ret=$?
			[ $ret -eq 0  ] && date +%s > $CDOPEN
		fi
		;;
	status)
		cat /proc/mounts | grep -q "$mountpoint" # check if mounted
		if [ $? -eq 0 ]; then
			Syslog "$DEVICE IS mounted"
			ret=0
		    elif [ -f $CDOPEN ] ; then
		       Syslog "$DEVICE tray open "
			   ret=2
		else
		    Syslog "$DEVICE NOT mounted "
            ret=1
		fi
		;;
	status_ipod)
		cat /proc/mounts | grep -q "$mountpoint_ipod" # check if mounted
		if [ $? -eq 0 ]; then
			Syslog "$DEVICE_IPOD IS mounted"
			ret=0
		else
		    Syslog "$DEVICE NOT mounted "
            ret=1
		fi
		;;
	*)
		Syslog "Unknown option $action"
		;;
esac

exit $ret
