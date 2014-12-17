#!/bin/sh

## start analysing the database as a daemon

# get the configs
. /etc/default/mediaconfigs

if [ -z ${MEDIA_DATABASE} ]; then
  echo "no Media database given in $CONFIGS"
  exit 2
fi


MediaAnalyzer --daemon --database ${MEDIA_DATABASE} #in background

