#!/bin/sh
#
# sleeptimer.sh: simple poweroff command

/bin/echo -e "HITK Power\nquit\n" | nc -q 1  127.0.0.1 2001
