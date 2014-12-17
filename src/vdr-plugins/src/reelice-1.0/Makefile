# Makefile for the ReelBox gstreamer output plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = reelice

### The object files (add further files here):

OBJS = $(PLUGIN).o ice_device.o ice_osd.o ice_setup.o

EXTRA_I18N := ice_types.h

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

#MAINSRC ?= ../../../../testing/src
VDRDIR = ../../..
LIBDIR = ../../lib
#VDRDIR = $(MAINSRC)/vdr-1.7
#LIBDIR = ../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common

### Includes and Defines (add further entries here):

### DirectFB
#INCLUDES += -I/usr/include/directfb
#LIBS     += -ldirectfb

### GStreamer
#LIBS += -lgstreamer-0.10

# use ICE includes/libs from reelbox-devel-ice, not SDK
INCLUDES += -I/usr/include/intelce
INCLUDES += -I/usr/include/intelce/linux_user
INCLUDES += `freetype-config --cflags`

LIBS     += -L/usr/lib/intelce
LIBS     += -lsrb -lgdl -lsystem_utils
LIBS     += -lismd_audio -lismd_bufmon -lismd_core -lismd_demux_v3 -lismd_viddec \
		-lismd_vidpproc -lismd_vidrend -lismd_clock_recovery

LIBS += -lpng

all: plug
clean: target-clean

target-clean:
	-rm -f *.o *.so *~ .dependencies
