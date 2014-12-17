#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.11 2005/10/03 14:05:20 ralf Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = setup

### The object files (add further files here):
OBJS = $(PLUGIN).o blockdevice.o config.o energyoptionsmenu.o formatmenu.o \
	mainmenu.o meminfo.o menus.o menueditrefreshitems.o menusysteminfo.o \
	netinfo.o networkfilesystem.o plugins.o setupmenu.o setupmultiroom.o \
	setupnetclientmenu.o setupnetworkfilesystemmenu.o setuprecordingdevicemenu.o \
	setupsetup.o setupwlanmenu.o resetsettingsmenu.o  systeminfo.o \
	util.o wlansetting.o
OBJS += menusetup_vdr.o

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR (taken from VDR's "config.h"):

VDRVERSION = $(shell sed -ne '/define VDRVERSION/ { s/^.*"\(.*\)".*$$/\1/; p }' $(VDRDIR)/config.h)
APIVERSION = $(shell sed -ne '/define APIVERSION/ { s/^.*"\(.*\)".*$$/\1/; p }' $(VDRDIR)/config.h)
ifeq ($(APIVERSION),)
	APIVERSION = $(VDRVERSION)
endif

VDRVERSNUM = $(shell sed -ne '/define VDRVERSNUM/ s/^.[a-zA-Z ]*\([0-9]*\) .*$$/\1/p' $(VDRDIR)/config.h)
APIVERSNUM = $(shell sed -ne '/define APIVERSNUM/ s/^.[a-zA-Z ]*\([0-9]*\) .*$$/\1/p' $(VDRDIR)/config.h)
ifeq ($(strip $(APIVERSNUM)),)
	APIVERSNUM = $(VDRVERSNUM)
endif

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

ifeq ($(shell test $(APIVERSNUM) -lt 10727 ; echo $$?),0)
  OBJS += i18n.o
endif

ifdef USEMYSQL
  OBJS += setupmysqlmenu.o
endif

#INCLUDES += -I$(VDRDIR)
INCLUDES += -I../../$(DVBDIR)/include

DEFINES += -D__STL_CONFIG_H 

ifdef DEBUG
  DEFINES += -DDEBUG_SETUP -DDEBUG
endif

LIBS += -lavahi-client

CXXFLAGS += -fno-strict-aliasing

#BDATE=$(shell date +%Y-%m-%d)
#DEFINES += -DBUILD="\"$(BUILD)\"" -DBDATE="\"$(BDATE)\""



### Allow user defined options to overwrite defaults:
include $(VDRDIR)/Make.common


### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN)"' $(INCLUDES) $<


### Aditional rules

$(I18Npot): $(wildcard *.cpp)

$(DEPFILE): Makefile vdrsetupclasses.h
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp)   > $@

libvdr-$(PLUGIN).so: vdrsetupclasses.h

vdrsetupclasses.h:  $(VDRDIR)/menu.c
	@echo "Extracting class definitions from VDR's menu.c for standard Setup"
	@./getVdrSetupMenu.pl $(VDRDIR)/menu.c

clean: clean-setup

clean-setup:
	@-rm -f vdrsetupclasses.h

useless-target-for-compatibility-with-vanilla-vdr:
	$(LIBDIR)/$@.$(APIVERSION)

