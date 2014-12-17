#!/bin/sh

DVDDEV="/dev/dvd"

. /etc/default/globals

(
	set -x
	smesgtl "Formating DVD"
	dvd+rw-format -force -blank $DVDDEV
	smesgtl "Formating DVD done."

	#FIXME: dvd+rw-format doesn't give the right exit level
	#if [ $? = 0 ] ; then
	#	smesgtl "Formating DVD successfull."
	#else
	#	smesgtl "Formating DVD failed."
	#fi
	eject $DVDDEV
) >/dev/null 2>&1 &
