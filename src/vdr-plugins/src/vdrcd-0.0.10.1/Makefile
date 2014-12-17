#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.2 2004/11/21 17:51:20 lordjaxom Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = vdrcd

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual

### The directory environment:

DVBDIR = ../../../DVB
VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR (taken from VDR's "config.h"):

APIVERSION = $(shell grep 'define APIVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include -I$(DVBDIR)/include -I../../../../DVB/include

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"' -D_GNU_SOURCE  -DHAVE_FILEBROWSER

ifdef RBLITE
DEFINES += -DRBLITE
endif

#DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"' -D_GNU_SOURCE 

### The object files (add further files here):

OBJS = $(PLUGIN).o i18n.o setup.o status.o suspend.o

ifeq ($(shell test -f $(VDRDIR)/PLUGINS/src/mp3/mp3.h ; echo $$?),0)
  DEFINES += -DHAVE_MP3_H
endif

ifeq ($(shell test -f $(VDRDIR)/PLUGINS/src/mp3/mplayer.h ; echo $$?),0)
  DEFINES += -DHAVE_MPLAYER_H
endif

ifeq ($(shell test -f $(VDRDIR)/PLUGINS/src/filebrowser/filebrowser.h ; echo $$?),0)
  DEFINES += -DHAVE_FILEBROWSER
endif
ifdef XEATRE
  DEFINES += -DXEATRE
endif

  DEFINES += -DHAVE_XINEMEDIAPLAYER


### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Targets:

all: libvdr-$(PLUGIN).so

libvdr-$(PLUGIN).so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude SCCS -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
