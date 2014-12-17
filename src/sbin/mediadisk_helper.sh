#!/bin/sh

# V 0.1: 2008-04-10 by RC
#
# $1: new device to mount
# $2: only for recording: yes|no
#
# return:
# 0 = ok
# 1 = mount failed
# 2 = not enough parameters

. /etc/default/globals


DEVICE=$1
MEDIADEVICE_ONLY_RECORDING=$2

if [ $# -lt 2 ]; then
	echo "you should not call this script directly. Its only for internal use."
	return 2
fi

# mountpoint
MP=/media/hd2
MEDIADIR=$MP
REC_PATH=$MP/recordings

if [ "$MEDIADEVICE_ONLY_RECORDING" = "yes" ]; then
	MP=$REC_PATH
	MEDIADIR=$MP
fi

# something mounted to MP?
if grep -q $MP /proc/mounts ; then
	umount -f $MP
fi

echo n | fsck -y $DEVICE


[ -d $MP ] || install -d -m 000 $MP

mount $DEVICE $MP

if [ $? -gt 0 ] ; then
	Syslog "mounting $DEVICE failed with $?."
	return 1
fi

for dir in music pictures recordings video; do
	[ -d $MEDIADIR/$dir ] || install -d -m2775 -g ftpusers $MEDIADIR/$dir
done

cat << EOF > /tmp/.mediadir
MEDIADIR=$MP
RECORDINGSDIR=$REC_PATH
EOF
