#!/bin/sh
#
# Creates a OSD screenshot using osddump / svdrp grab
#
# 2012-06-12: adjust for ReelBox ICE series (RC)
# 2007-02-07 by RollerCoaster


# include language
if [ -f /etc/sysconfig ] ; then
	. /etc/sysconfig
else
	. /etc/default/sysconfig
fi

if [ -f /etc/reel/globals ] ; then
	. /etc/reel/globals
else
	. /etc/default/globals
fi

echo running $0
if RbMini ; then
	Syslog "sorry, screenshots disabled on Reel NetClient."
	exit 0
fi

loglevel=3

syntax () {
cat << EOF
Usage: $MyName
Description:
     Creates a OSD screenshot using osddump
     Screenshots will be saved in /tmp

Options:
     no options

EOF
}

start() {

# check directory
test -d /media/reel/pictures/screenshot || mkdir /media/reel/pictures/screenshot

case $1 in
	"")
		nowhr=`date +%Y-%m-%d_%H.%M.%S`

		if RbLite ; then
			fname="/tmp/${nowhr}.jpg"
			osddump | pnmtojpeg > $fname
		elif RbIce ; then
			fname="/media/reel/pictures/screenshot/${nowhr}.jpg"
			svdrpsend.sh GRAB $fname
		else
			fname="/media/reel/pictures/screenshot/${nowhr}.png"
			hdfbshot -d $FBDEV -s0 $fname
		fi

		Log 1 "saved screenshot as $fname"
		smesgtl "Info: Screenshot saved in My Images / screenshot"
		;;
	-h)
		syntax
		exit 0
		;;
	*)
		echo "Unknown option $SOPTION."
		syntax
		exit 2
		;;
esac

}

#
# Main
#

MY_FB=$(grep hde_fb /proc/fb | awk '{print $1}')
FBDEV="/dev/fb$MY_FB"

start &
