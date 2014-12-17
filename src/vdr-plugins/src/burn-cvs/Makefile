#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.23 2006/10/01 21:26:44 lordjaxom Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = burn

### The object files (add further files here):

OBJS = burn.o chain-vdr.o chain-dvd.o jobs.o logger-vdr.o skins.o \
	   chain-archive.o manager.o menuburn.o menubase.o \
	   etsi-const.o tracks.o scanner.o gdwrapper.o iconvwrapper.o \
	   menuitems.o patchfont.o setup.o common.o config.o render.o \
	   genindex/pes.o

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -O3 -Wall -Woverloaded-virtual

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

ifdef DEBUG
	CXXFLAGS = -g -Wall -Woverloaded-virtual -fPIC
else
	DEFINES += -DNDEBUG
endif

ifdef RBLITE
DEFINES += -DRBLITE
endif

DEFINES += -DREPLEX_ONLY

### Includes and Defines (add further entries here):

LIBGD ?= ../../../../temp/docimage/libs/gd

INCLUDES += -I. -I$(LIBGD)
# $(shell gdlib-config --cflags)
# boost
#INCLUDES += -I/usr/include/boost
LIBS     += -L$(LIBGD)/.libs -lgd

SUBDIRS = proctools # tinyxml

LIBS += proctools/libproctools.a # tinyxml/libtinyxml.a


ifdef DEBUG_OSD
DEFINES += -DDEBUG_OSD
endif

ifndef TMPDIR
TMPDIR=/tmp
endif

ifndef DVDDEV
DVDDEV=/dev/dvd
endif

ifndef ISODIR
ISODIR=/pub/export
endif

DEFINES += -DTMPDIR='"$(TMPDIR)"' -DDVDDEV='"$(DVDDEV)"' -DISODIR='"$(ISODIR)"'

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common

#CXXFLAGS += -g
CXXFLAGS += -D_FILE_OFFSET_BITS=64

### Targets:

.PHONY: all dist clean SUBDIRS

all: burn-buffers

SUBDIRS:
	@for dir in $(SUBDIRS); do \
		make -C $$dir CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" lib$$dir.a ; \
	done

proctools/libproctools.a: SUBDIRS

libvdr-$(PLUGIN).so: $(OBJS) proctools/libproctools.a

burn-buffers: buffers.o
	$(CXX) $(CXXFLAGS) $< -o $@

scan-test: $(OBJS) proctools scan-test.o
	$(CXX) $(CXXFLAGS) scan-test.o $(OBJS) -o $@ \
		$(LIBS) -ljpeg -lpthread -ldl -lcap \
		$(shell ls $(VDRDIR)/*.o | grep -v vdr.o) $(VDRDIR)/libsi/libsi.a

gd-test: gdwrapper.o gdtest.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

clean: clean-plugin
clean-plugin:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f *.o genindex/*.o $(DEPFILE) *.so *.tgz core* *~ burn-buffers \
		scan-test test t/*.o
	@for dir in $(SUBDIRS); do \
		make -C $$dir clean ; \
	done


### Unit testing

TESTOBJS = t/main.o t/common_functions.o

test: $(TESTOBJS) $(OBJS) proctools
	$(CXX) $(CXXFLAGS) $(TESTOBJS) $(OBJS) -o $@ $(LIBS) \
		-lboost_unit_test_framework -ljpeg -lpthread -ldl -lcap \
		$(shell ls $(VDRDIR)/*.o | grep -v vdr.o) $(VDRDIR)/libsi/libsi.a
	./test

