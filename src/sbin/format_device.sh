#!/bin/bash
#
# 20100115: RC: automatically create an fstab entry after formating the device
#
#set -x

. /etc/default/globals

Syslog "This script is out-of-date. Use preparehd.sh instead. Abort."

exit 5



DEV=$1
FSTYPE=$2
LOGFILE=$3

. /etc/default/globals

#do paramater check here
#echo $#
if [ $# -ne 3 ]; then
	echo "failure" > $LOGFILE
	echo "Formatting failed. Invalid parameter."
fi
##less than 3 params report error

#echo $*
#echo $@

device=`echo $DEV | cut -f3 -d/`

(
Syslog "called with $@"
echo "start" > $LOGFILE

Syslog "Unmounting $DEV first before attempting to format"
#unmount before formatting
umount $DEV

#check the existance of mkfs.$FSTYPE
if command -v mkfs.$FSTYPE ; then
	echo mkfs.$FSTYPE $DEV
else
	echo "mkfs.$FSTYPE not found!"
	Syslog " ERROR: mkfs.$FSTYPE not found"
fi

reelfpctl -blinkled 4

case $FSTYPE in
	jfs)
		# mkfs.jfs is interactive, needs a Y to format
		mkfs.jfs -q $DEV
		;;
	xfs)
		mkfs.xfs -f $DEV
		;;
	*)
		mkfs.$FSTYPE $DEV
		;;
esac


if [ $? -eq 0 ]; then
	echo "success" >> $LOGFILE
	#get new uuid
	ID=$(blkid $DEV | awk '{print $2}')
	#get free mountpoint

	#create mountpoint

	# create fstab entry
	#mount device
	# create directories recordings etc.
	# create symlink to /media/reel
	msg=`tl "formatting %s completed." | sed s/%s/$device/`
	smesg "$msg"
else
	echo "failure" >> $LOGFILE
	msg=`tl "ERROR: formatting %s failed." | sed s/%s/$device/`
	smesg "$msg"
fi
reelfpctl -clearled 4

)&
