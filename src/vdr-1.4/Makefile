
# Makefile for the Video Disk Recorder
#
# See the main source file 'vdr.c' for copyright information and
# how to reach the author.
#
# $Id: Makefile 1.95 2006/08/20 10:44:22 kls Exp $

.DELETE_ON_ERROR:

CC       ?= ccache gcc
CFLAGS   ?= -g -O2 -Wall

CXX      ?= ccache g++
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual

LSIDIR   = ./libsi
TXMLDIR  = ./tinyxml
MANDIR   = /usr/local/man
BINDIR   = /usr/local/bin
SBINDIR  = /usr/sbin
LIBS     += -ljpeg -lpthread -ldl -lcap -lfreetype -lfontconfig
LOCDIR   = ./locale
INCLUDES += -I/usr/include/freetype2

PLUGINDIR    = ./PLUGINS
PLUGINLIBDIR = $(PLUGINDIR)/lib

ifdef RBLITE
  EXCLUDE_PLUGINS = -e dpkg -e dpkgopt -e ipod -e mediad -e netcv -e rbmini -e ripit \
		    -e skinreelng -e update -e webbrowser -e xxvautotimer
  DEFINES += -DRBLITE
else ifdef RBMINI
  EXCLUDE_PLUGINS = -e burn -e cdda -e formathd \
		    -e ipod -e ipkg -e mediad -e mpegplayer -e netcvrotor -e osdpip -e reelbox \
		    -e skinreelng -e update -e vdrcd -e webbrowser \
		    -e xepg -e xxvautotimer
  INCLUDES += -I/usr/arm-linux-gnueabi/include/freetype2
else
  # default settings, AVG + HTPC users
  EXCLUDE_PLUGINS = -e formathd -e ipkg -e mpegplayer -e rbmini -e update \
		    -e vdrcd -e xxvautotimer
  DEFINES += -DCAM_NEW
endif

VIDEODIR = /video
#DVBDIR   = ../kernel/linux-2.6.11.11/include

DOXYGEN  = /usr/bin/doxygen
DOXYFILE = Doxyfile

INCLUDES += -I$(DVBDIR)/include

-include ../Make.config

SILIB    = $(LSIDIR)/libsi.a
TXMLLIB  = $(TXMLDIR)/libtinyxml.a

OBJS = audio.o channels.o ci.o config.o cutter.o device.o diseqc.o dvbdevice.o dvbosd.o \
       dvbplayer.o dvbspu.o dvbsubtitle.o eit.o eitscan.o epg.o filter.o font.o i18n.o interface.o keys.o\
       lirc.o livebuffer.o menu.o menuitems.o mhwepg.o nit.o osdbase.o osd.o pat.o player.o plugin.o rcu.o\
       receiver.o recorder.o recording.o reelcamlink.o reelboxbase.o remote.o remux.o  \
       ringbuffer.o sdt.o sections.o skinclassic.o skins.o skinsttng.o sources.o spu.o status.o \
       svdrp.o themes.o thread.o providercharsets.o \
       timers.o tools.o transfer.o vdr.o videodir.o submenu.o help.o sysconfig_vdr.o dvdIndex.o suspend.o resumeDvd.o

ifdef USEMYSQL
  OBJS += vdrmysql.o
  LIBS += -lmysqlclient_r
endif

# SUBMENU + TinyXML +HELP
#OBJS += tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o submenu.o help.o

ifdef DEBUG
    DEFINES += -DDEBUG
endif

ifndef NO_KBD
DEFINES += -DREMOTE_KBD
endif

ifdef REMOTE
DEFINES += -DREMOTE_$(REMOTE)
endif

ifdef VDR_USER
DEFINES += -DVDR_USER=\"$(VDR_USER)\"
endif

ifdef ALWAYS_TS
    DEFINES += -DALWAYS_TS
endif

LIRC_DEVICE ?= /dev/lircd
RCU_DEVICE  ?= /dev/ttyS1

DEFINES += -DLIRC_DEVICE=\"$(LIRC_DEVICE)\" -DRCU_DEVICE=\"$(RCU_DEVICE)\" -DMAXOSDMEMORY="$(MAXOSDMEMORY)"

DEFINES += -D_GNU_SOURCE

DEFINES += -DVIDEODIR=\"$(VIDEODIR)\"
DEFINES += -DPLUGINDIR=\"$(PLUGINLIBDIR)\"
DEFINES += -DDELAYED_TRICKMODE
DEFINES += -DLOCDIR=\"$(LOCDIR)\"
#DEFINES += -DSVDRP_THREAD
#DEFINES += -DDEBUG_TIMES
# The version numbers of VDR and the plugin API (taken from VDR's "config.h"):

VDRVERSION = $(shell sed -ne '/define VDRVERSION/s/^.*"\(.*\)".*$$/\1/p' config.h)
APIVERSION = $(shell sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' config.h)

ifdef VFAT
# for people who want their video directory on a VFAT partition
DEFINES += -DVFAT
endif

H264_FRAME_PARSER=1
ifdef H264_FRAME_PARSER
      DEFINES += -DUSE_H264_PARSER
      OBJS += h264parser.o
endif

most: vdr i18n-vdr-dist plugins
all: vdr plugins i18n


# Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@echo Creating dependencies
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

# The main program:

vdr: $(OBJS) $(SILIB) $(TXMLLIB)
	@echo making $(@F)
	$(CXX) $(CXXFLAGS) -rdynamic $(OBJS) $(TXMLLIB) $(NCURSESLIB) $(LIBS) $(LIBDIRS) $(SILIB) -o vdr

dist: vdr i18n-vdr-dist

# The libsi library:

$(SILIB):
	$(MAKE) -C $(LSIDIR) all

# Internationalization (I18N):

PODIR     = po
LOCALEDIR = locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr.mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/vdr.pot

%.mo: %.po
	msgfmt -c $< -o $@

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --msgid-bugs-address='<reelbox-devel@mailings.reelbox.org>' -o $@ $^

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr.mo: $(PODIR)/%.po
	@rm -f $(PODIR)/$*.mo
	msgfmt -c $< -o $(LOCALEDIR)/$*/LC_MESSAGES/vdr.mo

#$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr.mo: $(PODIR)/%.mo
#	@mkdir -p $(dir $@)
#	@cp -av $< $@

.PHONY: i18n
i18n: i18n-vdr i18n-plugins
i18n-dist: i18n-vdr-dist i18n-plugins-dist

i18n-vdr: $(I18Nmsgs)

i18n-vdr-dist:
	@echo making $(@F)
	@for i in po/*.po ; do \
		odir=`echo $$i | cut -b4-8` ;\
		[ -d  $(LOCALEDIR)/$$odir/LC_MESSAGES ] || mkdir -p $(LOCALEDIR)/$$odir/LC_MESSAGES ;\
		echo msgfmt -c $$i -o $(LOCALEDIR)/$$odir/LC_MESSAGES/vdr.mo ;\
		msgfmt -c $$i -o $(LOCALEDIR)/$$odir/LC_MESSAGES/vdr.mo ;\
	done


i18n-plugins:
	@echo making $(@F)
	for i in `ls $(PLUGINDIR)/src | grep -ve '[^a-z0-9]' $(EXCLUDE_PLUGINS)`; do\
		$(MAKE) -C "$(PLUGINDIR)/src/$$i" i18n || exit 1 ;\
	done

i18n-plugins-dist:
	@echo making $(@F)
	for i in `ls $(PLUGINDIR)/src | grep -ve '[^a-z0-9]' $(EXCLUDE_PLUGINS)`; do\
		$(MAKE) -C "$(PLUGINDIR)/src/$$i" i18n-dist || exit 1 ;\
	done

install-i18n:
	@mkdir -p $(DESTDIR)$(LOCDIR)
	@(cd $(LOCALEDIR); cp -r --parents * $(DESTDIR)$(LOCDIR))

# The tinyxml library
$(TXMLLIB):
	$(MAKE) -C $(TXMLDIR) all

# The 'include' directory (for plugins):

include-dir:
	@mkdir -p include/vdr
	@(cd include/vdr; for i in ../../*.h; do ln -fs $$i .; done)
	@mkdir -p include/libsi
	@(cd include/libsi; for i in ../../libsi/*.h; do ln -fs $$i .; done)
	@mkdir -p include/tinyxml
	@(cd include/tinyxml; for i in ../../tinyxml/*.h; do ln -fs $$i .; done)

# Plugins:

plugins:
	for i in `ls $(PLUGINDIR)/src | grep -ve '[^a-z0-9]' $(EXCLUDE_PLUGINS)`; do\
		echo "making 'plug i18n' for '$$i'" ;\
		$(MAKE) -C "$(PLUGINDIR)/src/$$i" plug i18n-dist || exit 1 ;\
	done

orig-plugins-make-by-vdr-dont-use-it: include-dir
	@failed="";\
	noapiv="";\
	for i in `ls $(PLUGINDIR)/src | grep -v '[^a-z0-9]'`; do\
	    echo "Plugin $$i:";\
	    if ! grep -q "\$$(LIBDIR)/.*\$$(APIVERSION)" "$(PLUGINDIR)/src/$$i/Makefile" ; then\
	       echo "ERROR: plugin $$i doesn't honor APIVERSION - not compiled!";\
	       noapiv="$$noapiv $$i";\
	       continue;\
	    fi;\
	    $(MAKE) -C "$(PLUGINDIR)/src/$$i" all || failed="$$failed $$i";\
	done;\
	if [ -n "$$noapiv" ] ; then echo; echo "*** plugins without APIVERSION:$$noapiv"; echo; fi;\
	if [ -n "$$failed" ] ; then echo; echo "*** failed plugins:$$failed"; echo; fi

clean-plugins plugins-clean:
	@for i in `ls $(PLUGINDIR)/src | grep -v '[^a-z0-9]'`; do $(MAKE) -C "$(PLUGINDIR)/src/$$i" clean; done
	@-rm -f $(PLUGINDIR)/lib/*
	@-rm -f $(PLUGINDIR)/lib/.plugins-built

# Install the files:

install: install-bin install-plugins

# VDR binary:

install-vdr: install-bin

install-bin: vdr
	@mkdir -p $(SBINDIR)
	install -p vdr $(SBINDIR)

# Configuration files:

install-conf:
	@if [ ! -d $(VIDEODIR) ]; then\
	    mkdir -p $(VIDEODIR);\
	    cp *.conf $(VIDEODIR);\
	 fi

# Documentation:

install-doc:
	@mkdir -p $(MANDIR)/man1
	@mkdir -p $(MANDIR)/man5
	@gzip -c vdr.1 > $(MANDIR)/man1/vdr.1.gz
	@gzip -c vdr.5 > $(MANDIR)/man5/vdr.5.gz

# Plugins:
plugins-install: install-plugins

install-plugins: plugins
	@mkdir -p $(PLUGINLIBDIR)
	install -p $(PLUGINDIR)/lib/lib*-*.so.$(APIVERSION) $(PLUGINLIBDIR)

# Source documentation:

srcdoc:
	@cp $(DOXYFILE) $(DOXYFILE).tmp
	@echo PROJECT_NUMBER = $(VDRVERSION) >> $(DOXYFILE).tmp
	$(DOXYGEN) $(DOXYFILE).tmp
	@rm $(DOXYFILE).tmp

# Housekeeping:

clean: clean-vdr clean-plugins

clean-vdr vdr-clean:
	$(MAKE) -C $(LSIDIR) clean
	$(MAKE) -C $(TXMLDIR) clean
	-rm -f $(OBJS) $(DEPFILE) vdr core* *~
	-rm -rf srcdoc
	-rm -f .plugins-built

CLEAN: clean

