#!/bin/sh
# Here you can format the logfile as you like :)
logger -s "<$0> <$1> <$2>"
cp "$1" "$2"