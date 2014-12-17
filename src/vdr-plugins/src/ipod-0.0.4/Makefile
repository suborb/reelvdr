#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = ipod

### The object files (add further files here):

OBJS = $(PLUGIN).o menu.o parser.o list.o itunes.o \
	setupmenu.o playlist.o common.o

TSTOBJS = parser.o list.o itunes.o main.o common.o


### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common

### Includes and Defines (add further entries here):

INCLUDES +=

DEFINES += -DHAVE_XINEMEDIAPLAYER_PLUGIN

### Implicit rules:
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN)"' $(INCLUDES) $<

$(I18Npot): $(wildcard *.cc)

# Dependencies:
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cc) > $@


### Targets:

test: $(TSTOBJS)
	$(CXX) $(CXXFLAGS) $(TSTOBJS) -o $@

