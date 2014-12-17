#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.23 2006/10/01 21:26:44 lordjaxom Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = burn

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'const char\* plugin::VERSION *=' $(PLUGIN).c | awk '{ print $$5 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -O3 -Wall -Woverloaded-virtual

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

ifdef DEBUG
CXXFLAGS = -g -Wall -Woverloaded-virtual -fPIC
else
DEFINES += -DNDEBUG
endif

### For light systems - disable projectx and vdrsync
#ifdef REPLEX_ONLY
DEFINES += -DREPLEX_ONLY
#endif

### Check for HD Recordings ( Patch for VDR necessary )
ifdef BURN_USE_ISHD
DEFINES += -DUSE_ISHD
endif

### Use patchfont
ifdef USE_PATCHFONT
DEFINES += -DUSE_PATCHFONT
endif

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

#LIBGD ?= ../../../../temp/docimage/libs/gd
GD_INCLF ?= $(shell gdlib-config --cflags)
GD_LIBF  ?= $(shell gdlib-config --ldflags --libs)
INCLUDES += -I$(VDRDIR)/include -I. $(GD_INCLF)
# $(shell gdlib-config --cflags)
LIBS     += $(GD_LIBF) -lgd
DEFINES  += -DPLUGIN_NAME='"$(PLUGIN)"' -DPLUGIN_NAME_I18N='"$(PLUGIN)"' -D_GNU_SOURCE -D_LARGEFILE_SOURCE

### The object files (add further files here):

#OBJS = burn.o i18n.o menuburn.o menuitems.o common.o jobs.o process.o \
#       process-dvd.o process-vdr.o config.o setup.o pipe.o render.o   \
#       skins.o vdr_compat.o environment.o poller.o shell.o \
#	   logger-vdr.o chain-vdr.o chain-dvd.o

OBJS = burn.o chain-vdr.o chain-dvd.o jobs.o logger-vdr.o skins.o \
       chain-archive.o manager.o menuburn.o menubase.o \
       etsi-const.o tracks.o scanner.o gdwrapper.o iconvwrapper.o \
       menuitems.o setup.o common.o config.o render.o \
       genindex/pes.o

ifeq ($(shell test $(APIVERSNUM) -lt 10507 ; echo $$?),0)
  OBJS += i18n.o
endif


ifdef USE_PATCHFONTS
OBJS += patchfont.o
endif

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

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
LOCALEDIR = $(VDRDIR)/locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --msgid-bugs-address='<<vdr@smue.org>>' -o $@ $^

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@mkdir -p $(dir $@)
	cp $< $@

.PHONY: i18n
i18n: $(I18Nmsgs)

### Targets:

.PHONY: all dist clean SUBDIRS

all: libvdr-$(PLUGIN).so burn-buffers i18n

SUBDIRS:
	@for dir in $(SUBDIRS); do \
		make -C $$dir CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" lib$$dir.a ; \
	done

libvdr-$(PLUGIN).so: $(OBJS) SUBDIRS Makefile
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

burn-buffers: buffers.o
	$(CXX) $(CXXFLAGS) $< -o $@

scan-test: $(OBJS) proctools scan-test.o
	$(CXX) $(CXXFLAGS) scan-test.o $(OBJS) -o $@ \
		$(LIBS) -ljpeg -lpthread -ldl -lcap \
		$(shell ls $(VDRDIR)/*.o | grep -v vdr.o) $(VDRDIR)/libsi/libsi.a

gd-test: gdwrapper.o gdtest.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@ln -s $(ARCHIVE) $(TMPDIR)/$(PLUGIN)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE) $(PLUGIN)
	@-rm -rf $(TMPDIR)/$(ARCHIVE) $(TMPDIR)/$(PLUGIN)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f *.o genindex/*.o $(DEPFILE) *.so *.tgz core* *~ burn-buffers \
		scan-test test t/*.o
	@for dir in $(SUBDIRS); do \
		make -C $$dir clean ; \
	done

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Unit testing

TESTOBJS = t/main.o t/common_functions.o

test: $(TESTOBJS) $(OBJS) proctools
	$(CXX) $(CXXFLAGS) $(TESTOBJS) $(OBJS) -o $@ $(LIBS) \
		-lboost_unit_test_framework -ljpeg -lpthread -ldl -lcap \
		$(shell ls $(VDRDIR)/*.o | grep -v vdr.o) $(VDRDIR)/libsi/libsi.a
	./test

