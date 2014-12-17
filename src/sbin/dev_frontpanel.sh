#!/bin/sh

myname=`basename $0`

exec >> /dev/linkfrontpanel.log 2>&1
logger -t $myname start

CALLER=$(ps ax | grep "^ *$PPID" | awk '{print $5 " " $6 " " $7}')
echo I was called from $CALLER @ `cat /proc/uptime`


if grep -q 10027910 /proc/bus/pci/devices ; then
	# RB AVG II
	ln -sfv /dev/ttyS1 /dev/frontpanel
	logger -s -t $myname linking /dev/frontpanel to /dev/ttyS1
elif grep -q 808627a0 /proc/bus/pci/devices ; then
	# RB AVG I
	ln -sfv /dev/ttyS2 /dev/frontpanel
	logger -s -t $myname linking /dev/frontpanel to /dev/ttyS2
elif grep -q 80860709 /proc/bus/pci/devices ; then
	# RB ICE
	if ps ax | grep -v grep | grep -q reelbox-ctrld ; then
		echo "Error: reelbox-ctrld is already running. you will get wrong results."
		exit 1
	fi
	#check if /sbin/reelfpctl is executable
	[ ! -x /sbin/reelfpctl ] && ! ps ax | grep -v grep | grep avr_flasher && chmod 755 /sbin/reelfpctl

	ln -sfv /dev/ttyS0 /dev/frontpanel

	if ! [ -f /dev/.frontpanel.caps ]; then
		CAPS=$(reelfpctl -capability)
		echo $CAPS > /dev/.frontpanel.caps
	fi

	#just a cross check if it is AVG3
	reelfpctl -pwrctl 5a2100 > /dev/null
	PLATFORM=$(reelfpctl -pwrctl 5a2100 | awk '{print $4}')

	if [ "$PLATFORM" = "00" ] && ! grep AVR /dev/.frontpanel.caps ; then
		echo " AVR" >> /dev/.frontpanel.caps
	fi


	#result=$(reelfpctl -only-avr -getversion)
	#result=$(reelfpctl -capability | grep AVR)
	#if [ -z "$result" ]; then
	if ! grep -q AVR /dev/.frontpanel.caps ; then
		ln -sfv /dev/null /dev/frontpanel
	else
                #we have a AVG frontpanel
                echo ttyS0 > /dev/.have_frontpanel
	fi
else
	# no RB AVG
	ln -sfv /dev/null /dev/frontpanel
	logger -s -t $myname no RB Avantgarde - not linking /dev/frontpanel
fi

