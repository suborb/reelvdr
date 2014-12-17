#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile 1.8 2002/12/13 14:54:14 kls Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = ripit

### The object files (add further files here):

OBJS = $(PLUGIN).o ripitosd.o setup.o tools.o ripitstatus.o

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).h | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

CXXFLAGS += -fno-strict-aliasing 

target-for-compatibility-with-vanilla-vdr:
	$(LIBDIR)/$@.$(APIVERSION)
