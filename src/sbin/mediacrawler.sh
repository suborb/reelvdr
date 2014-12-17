#!/bin/sh

# crawl the given directory (and its subdirectories) and write into a database
# then analyse the database _once_

(
if [ ! -d "$1" ]; then
  echo "Error: no directory $1 found."
  exit 1
fi

DIR="$1" #DIR to add and analyse

# get the configs
. /etc/default/mediaconfigs

if [ -z ${MEDIA_DATABASE} ]; then
  echo "no Media database given in $CONFIGS"
  exit 2
fi

MediaCrawler --database ${MEDIA_DATABASE} --source "$DIR" # in foreground
&&
MediaAnalyzer --run-once --database ${MEDIA_DATABASE} #in foreground

) &
