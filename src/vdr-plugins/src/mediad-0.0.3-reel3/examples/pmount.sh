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

action="$1"
path="$2"

case "$action" in
mount)
  eject -t "$path"        # close the tray
  pmount -r "$path" || exit 1            # mount it
  ;;
unmount)
  pumount "$path" || exit 1           # unmount it
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

exit 0
