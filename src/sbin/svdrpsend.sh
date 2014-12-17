#!/bin/sh

DEST=127.0.0.1
PORT=2001
CMD=""
TIMEOUT=30

while [ $# -gt 0 ] ; do
        case "$1" in
                -d)
                        DEST=$2
                        shift 2
                ;;
                -p)
                        PORT=$2
                        shift 2
                ;;
                *)
                        if [ "$CMD" = "" ] ; then
			    CMD="$1"
			else
			    CMD="$CMD $1"
			fi
                        shift
                ;;
        esac
done

if [ "$CMD" = "" ] ; then
	echo no cmd
	exit 1
fi

#/bin/echo -e "$CMD\r\nQUIT" | nc -w $TIMEOUT $DEST $PORT
/bin/echo -e "$CMD\r\nQUIT" | nc -q 5 $DEST $PORT

