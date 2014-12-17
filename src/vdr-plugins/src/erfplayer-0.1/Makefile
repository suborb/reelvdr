#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = erfplayer

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp
BSPSHM = ../../../utils/bspshm
HDSHM = ../../../utils/hdshm3/src
REELBOX = ../reelbox

BSPINCLUDE = -I$(BSPSHM) -I$(BSPSHM)/include
HDINCLUDE = -I$(HDSHM) -I$(HDSHM)/include 
REELINCLUDE = $(BSPINCLUDE) $(HDINCLUDE) -I$(REELBOX)
LARGEFILE = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE 

RB_TARGET = /home/tiqq/rb/rb/opt/lib/vdr
 
### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR (taken from VDR's "config.h"):

VDRVERSION = $(shell grep 'define VDRVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')
APIVERSION = $(shell grep 'define APIVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')
ifeq ($(strip $(APIVERSION)),)
   APIVERSION = $(VDRVERSION)
endif

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include $(REELINCLUDE)

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'
DEFINES += -D__LINUX__

### The object files (add further files here):

OBJS = $(PLUGIN).o Player.o Channel.o rawplayer.o
#CXX      = g++-3.3

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) $(LARGEFILE) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Targets:

all: libvdr-$(PLUGIN).so

libvdr-$(PLUGIN).so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LDFLAGS) $(LIBS) -o $@
	cp $@ $(LIBDIR)/$@.$(APIVERSION)

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~

copy: all
	@cp libvdr-$(PLUGIN).so $(RB_TARGET)/libvdr-$(PLUGIN).so.$(APIVERSION)

testplayer: testplayer.o 
	$(CXX) $(CXXFLAGS) $(LARGEFILE) testplayer.o $(LDFLAGS) $(LIBS) -lpthread -o $@
testplayercopy: testplayer
	@cp testplayer /home/tiqq/rb/rb/hdp
testplayerclean: 
	@-rm -f testplayer.o
