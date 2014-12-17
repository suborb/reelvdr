#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#



### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp


LDFLAGS = -lbz2 -lz

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

ifdef REELVDR
OBJS = channelscan.o csmenu.o dirfiles.o filter.o scan.o transponders.o channellistbackupmenu.o rotortools.o
else
OBJS = channelscan.o csmenu.o filter.o scan.o transponders.o channellistbackupmenu.o rotortools.o
endif

ifdef REELVDR
PLUGIN = channelscan
else
PLUGIN = reelchannelscan
endif

-include $(VDRDIR)/Make.common

DEFINES += -DNDEBUG
#DEFINES += -DNDEBUG -DDEBUG_CHANNELSCAN=1
CXXFLAGS += -fno-strict-aliasing

INCLUDES += -I../../../$(DVBDIR)/include

ifeq ($(shell test -f $(VDRDIR)/PLUGINS/src/rotor/rotor.h; echo $$?),0)
  DEFINES += -DHAVE_ROTOR
endif

useless-target-for-compatibility-with-vanilla-vdr:
	$(LIBDIR)/$@.$(APIVERSION)

