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

ifeq ($(LRMDIR),)
 LRMDIR = /lib/linux-restricted-modules/`uname -r`
endif

MODULEPATH=     /lib/modules/${KERNELRELEASE}/extra

#bspshm-objs := bspshm.o

obj-m = hdshm.o

-include $(KERNELPATH)/Rules.make

all:
	echo $(KERNELPATH)
	$(MAKE) V=1 -C $(KERNELPATH) SUBDIRS=$(shell pwd) modules
	cp hdshm.ko hdshm_host.ko
clean:
	$(RM) *~ *.ko *.ko.cmd *.o.cmd *.o.cmd *.mod.c *.mod.o *.o .*cmd
	$(RM) -r .tmp_versions 

install: hdshm.mod.o hdshm.o
	install -p -D $^ $(LRMDIR)/hdshm
	lrm-manager

