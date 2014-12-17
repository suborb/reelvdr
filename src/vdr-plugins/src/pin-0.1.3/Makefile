#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.6 2006/05/20 15:24:24 root Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = pin

### The object files (add further files here):
OBJS    = $(PLUGIN).o setupmenu.o menu.o locks.o brdclocks.o rep.o msgreceiver.o talk.o
CMDOBJS = fskcheck.o talk.o

### User settings, list of plugins which can not protected
EXCLUDEPLUGINS = "netcv,netcvdiseqc,netcvinfo,reelbox,remote"

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

#directory used for fskcheck
FSKBINDIR ?= /usr/bin

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common


### Includes and Defines (add further entries here):

INCLUDES +=

DEFINES  += -D__EXCL_PLUGINS='""$(EXCLUDEPLUGINS)""'


### Targets:

install: fskcheck-install

fskcheck-install: fskcheck
	install -s $^ $(FSKBINDIR)

fskcheck: $(CMDOBJS)
	$(CXX) $(CXXFLAGS) $(CMDOBJS) -o $@

distclean: clean-fskcheck
clean-fskcheck:
	@-rm -f $(CMDOBJS) fskcheck

