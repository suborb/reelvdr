#!/bin/sh

# include language 
. /etc/default/sysconfig
. /etc/default/globals

#delete both resume.vdr (vdr 1.4) and resume (vdr 1.7)
rm $1/resume.vdr  $1/resume
touch /media/reel/recordings/.update

echo ""
tl "Info:"
echo ""
tl "This recording is marked as new."

exit 0


