#!/bin/sh
# handlearchived.sh - v.0.3
#
# source: burn-plugin
#
# add this lines to your reccmds.conf:
# folgende zeilen in die reccmds.conf eintragen:
#
# Clean archived recording   : /usr/local/bin/handlearchived.sh -clean
# Restore archived recording : /usr/local/bin/handlearchived.sh -restore
#
# history
# --------
# 20050318,ralf: first version
# 20061120, bugfix (ar)

DVDDEVICE=/dev/dvd
SETUPCONF=/etc/vdr/setup.conf

if [ "!" -f "$SETUPCONF" ]; then
    echo "Unable to find '$SETUPCONF' - please setup correct path in"
    echo "'$0'."
    exit 1
fi

. /etc/default/globals


printNoMark() {
	secho "Unable to find DVD archive mark - bailing out."
}

case "${1}" in
    -clean)
        if [ -f "$2/dvd.vdr" ]; then
            rm -f -- $2/[0-9][0-9][0-9].vdr "$2/index.vdr"
		secho "Recording has been cleaned up."
           else
		secho "Info: No mark found."
        fi
        ;;
    -delmark)
        if [ -f "$2/dvd.vdr" ]; then
            if [ -f "$2/index.vdr" -a -f "$2/001.vdr" ]; then
                rm -f -- "$2/dvd.vdr"
		secho "Recording is only on DVD - please restore recording first."
            else
		secho "Info: No mark found."
           fi
	fi
        ;;
    -restore)
        RECORDING="${2%/}"
        if [ "!" -f "$RECORDING/dvd.vdr" ]; then
            printNoMark
            exit 0
        fi
        RECMARK="${RECORDING##*/}"
        umount "$DVDDEVICE" >/dev/null 2>&1
        MOUNTDIR="/tmp/.restore.dvd.$$"
        mkdir -p "$MOUNTDIR"
        mount -o ro "$DVDDEVICE" "$MOUNTDIR"
        find "$MOUNTDIR" -type d -name "*.rec" | while read DVDRECORDING; do
	DVDRECMARK="${DVDRECORDING##*/}"
	if [ "$RECMARK" = "$DVDRECMARK" ]; then
		secho "The recording will be restored from DVD."
           ( cp -a -- $DVDRECORDING/[0-9][0-9][0-9].vdr "$DVDRECORDING/index.vdr" "$RECORDING"; umount "$DVDDEVICE"; rmdir "$MOUNTDIR" ) >/dev/null 2>&1 &
           fi
        exit 0
        done
        ;;
    *)
        echo "Illegal Parameter <$1>."
        ;;
esac
