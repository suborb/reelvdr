# dvbloop - A DVB Loopback Device
# Copyright (C) 2006 Christian Praehauser, Deti Fliegl
#-----------------------------------------
# File: Kbuild
# Desc: Makefile for the Linux kernel build system
# Date: October 2006
# Author: Christian Praehauser <cpreahaus@cosy.sbg.ac.at>, Deti Fliegl <deti@fliegl.de>
#
# This file is released under the GPLv2.
#
ifeq ($(obj),)
obj = .
endif

#################################################
# Version Check

ifneq ($(KERNELRELEASE),)

-include $(TOPDIR)/Rules.make

else

# take version info from last module build if available
-include $(obj)/.version

ifneq ($(SRCDIR),)
KDIR := $(SRCDIR)
else
ifneq ($(KERNELRELEASE),)
KDIR := /lib/modules/$(KERNELRELEASE)/build
else
KDIR := /lib/modules/$(shell uname -r|perl -ne 'if (/^([0-9]*)\.([0-9])*\.([0-9]*)(.*)$$/) { printf ("%s.%s.%s%s\n",$$1,$$2,$$3,$$4); };')/build

endif
endif

endif

obj-m += dvbloop.o
dvbloop-objs := dvblo.o dvblo_adap.o dvblo_adap_fe.o dvblo_char.o dvblo_util.o dvblo_adap_ca.o

PWD=`pwd`

ifneq ($(SRCDIR),)
EXTRA_CFLAGS = \
        -I$(SRCDIR)/drivers/media/dvb \
        -Idrivers/media/dvb -I$(@D)/../linux
else
EXTRA_CFLAGS = \
	-I/usr/src/linux/drivers/media/dvb \
	-Idrivers/media/dvb -I$(@D)/../linux
endif

default: build

test:
	gcc -g -lpthread -o dvblo_test -D_GNU_SOURCE dvblo_test.c

load:
	modprobe dvb_core
	insmod ./dvbloop.ko

unload:
	rmmod dvbloop
	
reload: unload load

build:
	make -C $(KDIR) M=$(PWD) modules

install: build
	make -C $(KDIR) M=$(PWD) modules_install

help:
	make -C $(KDIR) M=$(PWD) help

clean:
	@rm -f *~ dvblo_test
	make -C $(KDIR) M=$(PWD) clean
