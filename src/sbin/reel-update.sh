#!/bin/sh

#
# Reelbox Internet Iso-Updater
#
# (C) 2007 by Reel Multimedia AG http://www.reelbox.org
# Author: rollercoaster@reel-multimedia.com
# Licence: GPL2
# History: 20071014 	initial release
#
# Version: 0.0.1
#

cat << EOF
ReelBox Internet Updater
(c) 2007 by Reel Multimedia AG http://www.reelbox.org
Author: rollercoaster@reel-multimedia.com
Licence: GPL2
EOF

PATH="/usr/sbin:/bin:/usr/bin:/sbin"


# Check sysconfig
if [ -d /etc/default ] ; then
	. /etc/default/sysconfig
else
	. /etc/sysconfig
fi

# include language
. /etc/default/globals

#UPDT_SERVER=""
#UPDT_PROTO=""
#UPDT_PATH=""
#UPDT_BRANCH=""

# Server and Directory
if [ "$UPDT_SERVER" = "" ]; then
	UPDT_SERVER="www.reelbox.org"
fi

if [ "$UPDT_PROTO" = "" ]; then
	UPDT_PROTO="http"
fi

if [ "$UPDT_PATH" = "" ]; then
	UPDT_PATH="software/ReelBox"
fi

if [ -f /etc/testing ]; then
	UPDT_BRANCH="testing"
elif [ "$UPDT_BRANCH" = "" ]; then
	UPDT_BRANCH="stable"
fi

versionf="/etc/release"
cdromdev="/dev/scd0"
cdmp="/media/dvd"

# programs

APT=aptitude
#SIMULATE="--simulate"
DPKG="dpkg $SIMULATE --refuse-downgrade --skip-same-version --force-confold"


Cdeject () {
	eject -r
}

CheckInternet () {
	if [ ! "$USE_PROXY" = "yes" ] ;then
		smesgtl "Checking internet connection"
		if ! ping -c 3 $UPDT_SERVER >/dev/null 2>&1 ; then
			sleep 2
			smesgtl "No internet connection. Update aborted!"
			exit 1
		fi
	fi
}

DoCdUpdate () {
	# check files
	pcount=`ls $cdmp/*_i386.deb | wc -w`

	if [ $pcount = 0 ] ; then
		smesgtl "no packages on CD. Exit."
		Cdeject
		return 1
	else
		smesg $(tl "found %s packages on CD. Installing..." | sed s/%s/$pcount/) 
	fi

	# install files
	$DPKG -i $cdmp/*_i386.deb
	#umount cd
	if [ $? = 0 ] ; then
		smesgtl "upgrade done."
	else
		smesgtl "upgrade failed."
	fi
	Cdeject
}

DoUpdate () {
	smesgtl "Fetching list from server"
	apt-get -y update
}

DoUpgrade () {
	smesgtl "Doing upgrade. This can take some time..."
	# apt-get is crap. aptitude is more modern, so it is worse.
	# if anyone who is in charge for apt*: pls implement a *useable*
	#     function for unattended updates!
	echo -e "n\nn\nn\nn\nn\nn\nn\nn\nn\nn" > /tmp/sayno
	# configure pending packages
	dpkg --configure -a < /tmp/sayno
	# do the reel update
	aptitude -f -y dist-upgrade < /tmp/sayno | tee /tmp/apt.log
	err=$?
	count=`grep "packages upgraded," /tmp/apt.log | cut -f1 -d" "`
	if [ $err = 0 ] ; then
		msg=`tl "upgrade done. upgraded %s packages." | sed s/%s/$count/`
		smesg "$msg"
	else
		smesgtl "upgrade failed."
	fi
}



#
# Main
#

set -x


# Start here
case $1 in
	cd_update)
		# mount cd
		if ! grep -q $cdromdev /proc/mounts ; then
			smesgtl "mounting cdrom"
			mount $cdromdev
		else
			echo "already mounted"
			#TODO: check mount path
		fi

		case $? in
			0)
				;;
			32)
				smesgtl "No medium found. Abort."
				exit 1
				;;
			*)
				smesgtl "mounting failed (bad cd?). Abort."
				exit 1
				;;
		esac

		if [ -x $cdmp/doupdate.sh ] ; then
			$cdmp/doupdate.sh
		else
			DoCdUpdate
		fi
		;;
	inet_update)
		CheckInternet
		DoUpdate
		DoUpgrade
		;;
	*)
		echo "You should not call me directly. Use the setup menu instead."
		exit 1
	;;
esac
