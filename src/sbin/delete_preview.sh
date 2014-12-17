#!/bin/sh

. /etc/default/globals

# go to recordings directory
cd "$1"

PREVIEW_IMAGE=preview_vdr.png

if [ -e $PREVIEW_IMAGE ]
then
    rm -f "$PREVIEW_IMAGE" && tl "Preview deleted" || tl "Error deleting preview"
    return
fi

tl "Preview not found"

