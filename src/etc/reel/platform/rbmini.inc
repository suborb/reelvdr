REELFPCTL="/sbin/reelfpctl -nowait"
HDCONTROL=true
FPLEDS=/etc/init.d/fpleds

PrepareActive () {
	# enable the display
	rbmdisp 1
	# show the splash screen
	zcat /usr/share/reel/boot-client_720x576_24.ppm.gz | /usr/sbin/ppmtofb  -
}

FPSetStandby () {
	if [ "$FPLEDSTBOFF" = "yes" ]; then
		$REELFPCTL -clearled $ALL_LED
	else
		$REELFPCTL -clearled $ALL_LED -setled $(($STANDBY_LED+$REMOTE_LED))
	fi
}

