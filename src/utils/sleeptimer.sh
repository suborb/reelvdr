#!/bin/sh
#
# sleeptimer.sh: simple poweroff command

echo -e "HITK Power\nquit\n" | nc 127.0.0.1 2001
