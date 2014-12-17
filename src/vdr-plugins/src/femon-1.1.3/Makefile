#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# Debugging on/off
#FEMON_DEBUG = 1

# NTSC on/off
#FEMON_NTSC = 1

# Strip debug symbols?  Set eg. to /bin/true if not
STRIP = strip

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = femon

### The object files (add further files here):

OBJS = femon.o femonosd.o femonreceiver.o femoncfg.o femontools.o

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

ifdef FEMON_NTSC
DEFINES += -DNTSC
endif

ifdef FEMON_DEBUG
DEFINES += -DDEBUG
endif

INCLUDES += -I../../$(DVBDIR)/include
