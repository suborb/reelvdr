#
# Makefile for the kernel DVB device drivers.
#
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

ifeq ($(KERNEL_HEADERS),)
  KERNEL_HEADERS = /lib/modules/$(KERNELRELEASE)/build/
endif

ifeq ($(DESTDIR),)
 DESTDIR=
endif

#EXTRA_CFLAGS = -I$(XXX)/drivers/media/dvb/dvb-core/ -I$(XXX)/include
#EXTRA_CFLAGS = -I../../linux-2.6.11.11/include/linux/dvb
EXTRA_CFLAGS = -I$(KERNEL_HEADERS)/include/linux/dvb -I$(DVB_DRIVER)

#MODULEPATH=     /lib/modules/${KERNELRELEASE}/net
MODULEPATH=     /lib/modules/${KERNELRELEASE}/kernel/drivers/media/dvb


dvb-core-objs = dvbdev.o dmxdev.o dvb_demux.o dvb_filter.o \
	        dvb_ca_en50221.o dvb_frontend.o \
		dvb_net.o dvb_ringbuffer.o \


obj-m += dvb-core.o
obj-m += dvb-pll.o
obj-m += stv0299.o
obj-m += tda1004x.o
obj-m += tda10023.o
obj-m += stv0288.o
obj-m += cx24116.o
 
-include $(KERNELPATH)/Rules.make

all:
	echo $(KERNELPATH)
	$(MAKE) V=1 -C $(KERNELPATH) SUBDIRS=$(shell pwd) modules

clean:
	$(RM) *~ *.ko *.ko.cmd *.o.cmd *.o.cmd *.mod.c *.mod.o *.o .*cmd
	$(RM) -r .tmp_versions 
