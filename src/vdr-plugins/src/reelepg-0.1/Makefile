#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.7 2006/06/18 13:59:36 schmitzj Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = reelepg

### The object files (add further files here):

OBJS = $(PLUGIN).o config.o timeline.o pngutils.o
ifdef USE_TFT
DEFINES += -DUSE_TFT
ifneq ($(shell which freetype-config),)
INCLUDES += $(shell freetype-config --cflags)
LIBS += $(shell freetype-config --libs)
else
INCLUDES += -I/usr/include/freetype -I/usr/local/include/freetype
LIBS += -lfreetype
endif
OBJS += font.o
endif

LIBS += -lpng -lcurl

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The version number of VDR's plugin API (taken from VDR's "config.h"):

APIVERSION = $(shell sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' $(VDRDIR)/config.h)

RB_TARGET = /usr/lib/vdr

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

### Includes and Defines (add further entries here):

INCLUDES += -I$(DVBDIR)/include

DEFINES += -DDEFAULT_IMG_PATH="\"/usr/share/reel/skinreel\""


