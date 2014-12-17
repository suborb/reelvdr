#!/bin/bash
# This script is based on the mp3 plugin's mount.sh script
#
# This script is called from VDR to mount/unmount/eject
# devices detected with the mediad plugin
#
# argument 1: wanted action, one of mount,unmount,eject,status
# argument 2: mountpoint to act on
#
# mount,unmount,eject must return 0 if succeeded, 1 if failed
# status must return 0 if device is mounted, 1 if not
#

echo "$0 $*"
SPEED=12
SPEEDCONTROL="speedcontrol -x$SPEED"

action="$1"
path="$2"

case "$action" in
mount)
  $SPEEDCONTROL
  eject -t "$path" 2>/dev/null        # close the tray

  # pmount takes too long when 'no medium' in dvd drive
  # so check if the dvd drive is empty; if so exit with 
  # error before call to pmount
  if [ "/dev/dvd" == "$path" ]; then
     echo " _________  reading $path START _______________"
     #exec 3<&0
     exec 0<"/dev/dvd"  # read the dvd, if return val == 0 then 'medium available' 
                        # else do not call pmount as it takes too long to quit 
                        # when 'no media' is in the dvd drive
     ret=$?
     echo "ret $ret"
     if [ $ret -ne 0 ]; then
        echo "_________ failed reading DVD _________END__" 
        exit 1
     fi
     #exec 0<&3
     echo " _________ reading $path  END ________"
  fi

  echo " ==========START pmount============"
  pmount -r "$path" 2>/dev/null || exit 1      # mount it
  ;;
unmount)
  pumount "$path" 2>/dev/null || exit 1        # unmount it
  ;;
eject)
  eject "$path" || exit 1            # eject disk
  ;;
status)
  cat /proc/mounts | grep -q "$path" # check if mounted
  if [ $? -ne 0 ]; then              # not mounted ...
    exit 1
  fi
esac

echo "$0 end $ret"
exit 0
