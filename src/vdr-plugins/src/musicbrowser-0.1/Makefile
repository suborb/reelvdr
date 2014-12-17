#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = musicbrowser

### The object files (add further files here):

OBJS = $(PLUGIN).o filecache.o filetools.o browserItems.o\
	playlist.o  convert.o browserbase.o browserStatus.o browsermenu.o\
        userIf.o searchIf.o playlistIf.o optionsIf.o cover.o

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Woverloaded-virtual -Wextra

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
INCLUDES += -I. -I/usr/arm-linux-gnueabi/include/taglib
LDFLAGS += -L/usr/arm-linux-gnueabi/lib -ltag 
else
INCLUDES += -I. -I/usr/include/taglib

INCLUDES +=`taglib-config --cflags`
LDFLAGS  +=`taglib-config --libs`
endif

LDFLAGS += -L../../../libs/reelaudiodb-0.1 -lreelsearchapi
$(I18Npot): $(PLUGIN).h

target-for-compatibility-with-vanilla-vdr:        
	$(LIBDIR)/$@.$(APIVERSION)
