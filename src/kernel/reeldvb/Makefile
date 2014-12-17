ifndef CROSSPREFIX
 CROSSPREFIX=
endif

CC?=${CROSSPREFIX}gcc
LD?=${CROSSPREFIX}ld

ifeq ($(KERNELRELEASE),)
 KERNELRELEASE= $(shell uname -r)
endif

ifneq ($(findstring 2.6,$(KERNELRELEASE)),)
 export-objs:=
 list-multi:=
 KMODSUF:= ko
else
 KMODSUF:= o
endif

ifeq ($(KERNELPATH),)
 KERNELPATH=    /lib/modules/${KERNELRELEASE}/build
endif

ifeq ($(DESTDIR),)
 DESTDIR=
endif

DVB_DRIVER=$(PWD)/../dvb-kernel


EXTRA_CFLAGS = -I$(DVB_DRIVER)

MODULEPATH=     /lib/modules/${KERNELRELEASE}/net

reeldvb-objs := reel_core.o reel_hw.o reel_i2c.o reel_fpga.o reel_ci.o reel_frontend.o

obj-m = reeldvb.o

-include $(KERNELPATH)/Rules.make

all:
	echo $(KERNELPATH)
	$(MAKE) V=1 -C $(KERNELPATH) SUBDIRS=$(shell pwd) modules

clean:
	$(RM) *~ *.ko *.ko.cmd *.o.cmd *.o.cmd *.mod.c *.mod.o *.o .*cmd
	$(RM) -r .tmp_versions 
