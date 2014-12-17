#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = avahi

### The object files (add further files here):

OBJS = $(PLUGIN).o avahithread.o avahilog.o
#OBJS = $(PLUGIN).o i18n.o avahithread.o avahilog.o

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Woverloaded-virtual

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common


### Includes and Defines (add further entries here):


DEFINES  += -D__STL_CONFIG_H

ifdef RBMINI
  INCLUDES += -I/usr/arm-linux-gnueabi/include/dbus-1.0 -I/usr/arm-linux-gnueabi/lib/dbus-1.0/include
  LDFLAGS  += -lavahi-client -L/usr/arm-linux-gnueabi/lib -ldbus-1 -lpthread -lrt
else
  INCLUDES += `pkg-config --cflags dbus-1`
  LDFLAGS  += -lavahi-client `pkg-config --libs dbus-1`
endif

$(I18Npot):

target-for-compatibility-with-vanilla-vdr:        
	$(LIBDIR)/$@.$(APIVERSION)
