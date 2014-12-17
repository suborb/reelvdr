#!/bin/bash

# Change this to your liking
PTPLOGFILE="/var/log/ptpmount.log"
PTPmtab="/var/run/PTPmtab"
MOUNTPATH="/media"

. /etc/default/globals # for smesgtl else use

 #works for Sony DSC-T50
GetVendor()
{
	vendor="";
	if test -r "/sys$DEVPATH/device/vendor"; then
	vendor="`cat \"/sys$DEVPATH/device/vendor\"`"
	elif test -r "/sys$DEVPATH/device/manufacturer"; then
	vendor="`cat \"/sys$DEVPATH/device/manufacturer\"`"
	fi
	vendor="`echo \"$vendor\" | sed 's/^ *//; s/ *$//'`"
} #end GerVendor


GetModel()
{ 
	model="";
	if test -r "/sys$DEVPATH/device/model"; then
	model="`cat \"/sys$DEVPATH/device/model\"`"
	elif test -r "/sys$DEVPATH/device/product"; then
	model="`cat \"/sys$DEVPATH/device/product\"`"
	fi
	model="`echo \"$model\" | sed 's/^ *//; s/ *$//'`"
	#export vendor model
} #end GetModel


DoMount()
{
	GetModel
	#GetVendor
	
	newmodel=`echo "$model" | awk '{ if(NF>3) {print $1, $2, $3} else {print $0}}'`
	model=$newmodel; #update model name
	#Check if the model name has more than three words, 
	#if so truncate to three words else leave it unchanged
	
	MOUNTDIR=$model
	MOUNTPOINT="$MOUNTPATH""/$MOUNTDIR"
	
	echo "ADD called" >> $PTPLOGFILE
	
	#TRY and get a MOUNTPOINT
	newMOUNTPOINT=$MOUNTPOINT
	n=1
	while [ $n -lt 10 ]; do
		if [ -e "$newMOUNTPOINT" ]; then
			newMOUNTPOINT="${MOUNTPOINT}$n"
			echo "Trying $newMOUNTPOINT" >> $PTPLOGFILE
		else
			break;
		fi
		let n=n+1
		#let $((n++))
	done
	#Can fail! So, check again and give up mounting
	
	MOUNTPOINT=$newMOUNTPOINT
	
	if [ ! -e "$MOUNTPOINT" ]; then
		install -m 000 -d "$MOUNTPOINT" ;
		gphotofs "$MOUNTPOINT" -o allow_other
		echo "$USB_BUS:$USB_DEV:$MOUNTPOINT:$model:" >> $PTPmtab
		echo "PTP camera mounted in $MOUNTPOINT">> $PTPLOGFILE;
		ls -l "$MOUNTPOINT" > /dev/null 2>&1 #to speed up access when user opens the directory for the first time.
		
		#vdr message
		smesg $(tl "%s Camera added" | sed s/%s/$model/)
		#start filebrowser
		sudo svdrpsend.sh plug filebrowser OPEN "$MOUNTPOINT"
	else 
		echo "$MOUNTPOINT exists! Camera not mounted" >> $PTPLOGFILE;
		smesg $(tl "%s Camera detected, but not mounted" | sed s/%s/$model/)
	fi

} # end DoMount

DoUnmount()
{
	echo "REMOVE called" >> $PTPLOGFILE
	
	MOUNTPOINT=`grep ^$USB_BUS:$USB_DEV: $PTPmtab | awk -F: '{print $3}'`
	modelname=`grep ^$USB_BUS:$USB_DEV: $PTPmtab | awk -F: '{print $4}'` 
	
	
	if [ -d  "$MOUNTPOINT" ];then
		fusermount -u  "$MOUNTPOINT" ;
		rmdir  "$MOUNTPOINT" ; 
		echo "Unmounted PTP Camera from $MOUNTPOINT" >> $PTPLOGFILE
		# remove the PTPmtab entry
		#TODO: use sed
		grep -v ^$USB_BUS:$USB_DEV: $PTPmtab > $PTPmtab.tmp;
		cp $PTPmtab.tmp $PTPmtab
		
		#vdr messages
		smesg $(tl "%s Camera removed" | sed s/%s/$model/)
	else
		echo "MountPoint: $MOUNTPOINT doesnot exist" >> $PTPLOGFILE
	fi

} #end DoUnmount

Debug()
{
	(echo "----START-DEBUG------";
         set #get all environmental variables
         echo "----END-DEBUG-----") >> $PTPLOGFILE
}
Syntax()
{
	echo "Usage:"
	echo "$0 add|remove"
	echo "to add/remove PTPCamera"
	echo
} #end Syntax

case "$1" in
	add)
		DoMount
		;;
	remove)
		DoUnmount
		;;
	debug)
		Debug
		;;
	*)
		Syntax
		;;
esac

