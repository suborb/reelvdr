#!/bin/sh

syntax () {
cat << EOF
Usage: `basename $0` <filesystem> <hd device>
Options:
     filesystem: use one of: reiserfs | jfs
     hd device:  /dev/hdc or /dev/hdd

Description:
     Prepares unformatated HD for ReelBox
EOF
}


### Main

if [ $# -lt 2 ] ; then
	syntax
	exit 1
fi

FS=$1
HDD=$2

hdparm -d1 -q $HDD
killall swapd
sleep 2

umount ${HDD}1

# blank bootblock+partition table
dd if=/dev/zero of=${HDD} bs=512 count=1

fdisk ${HDD} < /usr/share/reel/fdiskcommands


eval mkfs.$FS -q ${HDD}1
mount ${HDD}1 /media/hd

for i in images recordings music pictures video opt ; do
	install -d -m777 /media/hd/$i
done

touch /media/hd/video/.update

