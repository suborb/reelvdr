#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.21 2007/12/31 06:34:02 amair Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = skinreel3

# Debugging on/off
# SKINREEL_DEBUG = 1

# If you have installed ImageMagick and want to use
# images in event's and recording's details.
HAVE_IMAGEMAGICK = 1

# If you are using the epgsearch plugin and want to see the number of
# timer conflicts in the main menu's info area.
#SKINREEL_USE_PLUGIN_EPGSEARCH = 0

# If you use the mailbox plugin this will include support for it.
#SKINREEL_USE_PLUGIN_MAILBOX = 0

# Include code to support the Avards plugin:
#  - Dynamic OSD size depending on current WSS mode.
#  - Display current WSS mode in channel info and replay OSDs.
#SKINREEL_USE_PLUGIN_AVARDS = 0

# Disable any code that is used for scrolling or blinking text.
# NOTE: this is only useful if you want to save some bytes because you
# can disable them in the setup too.
SKINREEL_DISABLE_ANIMATED_TEXT = 0

# Set the descriptions for fonts you've patched in VDR. These fonts then
# can be selected in ReelNG setup.
#SKINREEL_FONTS = "\"Test Font\", \"Test2 Font\""

# If you have installed FreeType2 and want to use TrueTypeFonts.
HAVE_FREETYPE = 1

# Strip debug symbols?  Set eg. to /bin/true if not
#STRIP = strip
STRIP = /bin/true

# Make use of pngutils to dispaly epg iamges
USE_PNGUTILS=1

### The C++ compiler and options:

CXXFLAGS += -fPIC

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:
#TODO
CLEAR_BUG_WORKAROUND = 1


### The object files (add further files here):

OBJS = $(PLUGIN).o reel.o config.o logo.o tools.o status.o texteffects.o setup.o displaymenu.o displaychannel.o displayreplay.o displayvolume.o displaytracks.o displaymessage.o mytextscroller.o


### Includes and Defines (add further entries here):

ifdef REELVDR
  DEFINES += -DREELVDR
endif

ifdef SKINREEL_USE_PLUGIN_EPGSEARCH
  DEFINES += -DUSE_PLUGIN_EPGSEARCH
else
# for backwards compatibility only
ifdef SKINREEL_HAVE_EPGSEARCH
  DEFINES += -DUSE_PLUGIN_EPGSEARCH
endif
endif

ifdef SKINREEL_DEBUG
  DEFINES += -DDEBUG
endif

ifdef SKINREEL_USE_PLUGIN_MAILBOX
  DEFINES += -DUSE_PLUGIN_MAILBOX
endif

ifdef SKINREEL_USE_PLUGIN_AVARDS
  DEFINES += -DUSE_PLUGIN_AVARDS
endif

ifdef HAVE_IMAGEMAGICK
  DEFINES += -DHAVE_IMAGEMAGICK
endif

DEFINES += -DRECORDING_COVER='"Cover-Reel"'

ifdef SKINREEL_DISABLE_ANIMATED_TEXT
  DEFINES += -DDISABLE_ANIMATED_TEXT
endif

ifdef CLEAR_BUG_WORKAROUND
  DEFINES += -DCLEAR_BUG_WORKAROUND
endif

DEFINES += -DSKINREEL_FONTS=$(SKINREEL_FONTS)

INCLUDES +=


### common config
include $(VDRDIR)/Make.config

ifdef HAVE_IMAGEMAGICK
    OBJS += bitmap.o
    ifdef RBMINI
        LIBS += -lMagick++
    else
    ifneq ($(shell which GraphicsMagick++-config),) 
	INCLUDES += $(shell GraphicsMagick++-config --cppflags)
	LIBS     += $(shell GraphicsMagick++-config --libs)
    else
        LIBS += -lMagick++
    endif
    endif
endif

ifdef USE_PNGUTILS
  OBJS +=  pngutils.o
  LIBS += -lpng -lcurl -ljpeg
endif


ifdef HAVE_FREETYPE
	ifneq ($(shell which freetype-config),)
		INCLUDES += $(shell freetype-config --cflags)
		LIBS += $(shell freetype-config --libs)
	else
		INCLUDES += -I/usr/include/freetype -I/usr/local/include/freetype
		LIBS += -lfreetype
	endif
	DEFINES += -DHAVE_FREETYPE
	OBJS += font.o
endif

ifdef RBMINI
   INCLUDES += -I/usr/arm-linux-gnueabi/include/freetype2
endif

include $(VDRDIR)/Make.common


#i18n compatibility generator:
#i18n.c: i18n-template.c buildtools/po2i18n.pl $(I18Npo)
#	buildtools/po2i18n.pl < i18n-template.c > i18n.c

useless-target-for-compatibility-with-vanilla-vdr:
	$(LIBDIR)/$@.$(APIVERSION)
