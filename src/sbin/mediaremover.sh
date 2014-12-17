#!/bin/sh

# remove all the files in a given directory from the database

(

# get the configs
. /etc/default/mediaconfigs

if [ -z ${MEDIA_DATABASE} ]; then
  echo "no Media database given in $CONFIGS"
  exit 2
fi

if [ -z "$1" ]; then
  echo "Error: no directory specified"
  exit 1
fi


MediaRemove --database ${MEDIA_DATABASE} "$1"

### remove the given path from the database
## mysql --reeluser=reeluser --password=reeluser ${MEDIA_DATABASE} -e "DELETE metadata WHERE path like \'$1%\'"
##
### MediaCleaner --database ${MEDIA_DATABASE}

) &
