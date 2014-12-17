#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = vlcclient

### The object files (add further files here):

OBJS = socket.o setup.o common.o menu.o remote.o patchfont.o player.o i18n.o $(PLUGIN).o

# unset this if you want a detailed setup menu 
SIMPLE_SETUP=1

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

DVBDIR = ../../../../DVB
VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common


ifdef SIMPLE_SETUP
DEFINES += -DSIMPLE_SETUP
endif

