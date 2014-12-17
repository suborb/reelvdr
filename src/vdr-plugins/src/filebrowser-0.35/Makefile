#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = filebrowser

### The object files (add further files here):

OBJS = $(PLUGIN).o mountmanager.o menuBrowserBase.o menuBrowser.o fileType.o dirType.o \
	stillpictureplayer.o convert.o filecache.o menufileinfo.o browserItems.o \
	threadprovider.o menuplaylist.o playlist.o playlistUserIf.o browserStatus.o \
	userIf.o userIfBase.o filetools.o setup.o menuDirSelect.o

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
INCLUDES += -I. -I/usr/arm-linux-gnueabi/include/taglib
LDFLAGS += -L/usr/arm-linux-gnueabi/lib -ltag 
else
INCLUDES += -I. -I/usr/include/taglib

INCLUDES +=`taglib-config --cflags`
LDFLAGS  +=`taglib-config --libs`
endif

$(I18Npot): $(PLUGIN).h

target-for-compatibility-with-vanilla-vdr:        
	$(LIBDIR)/$@.$(APIVERSION)
