#
# Makefile for speedcontrol
#
#

CC ?= gcc
CFLAGS ?= -O2

BINPATH = /usr/sbin



all: speedcontrol


speedcontrol: speedcontrol.c
	$(CC) $(CFLAGS) -o $@ $^

install: speedcontrol
	install -m 755 $< $(BINPATH)

uninstall:
	rm -f $(BINPATH)/speedcontrol

clean:
	rm -f *.o speedcontrol
