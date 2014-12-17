#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = reelbox

# set it if you want to compile the skin for use with the reelbox
REELSKIN=1

### The object files (add further files here):

OBJS = $(PLUGIN).o ac3.o AudioDecoder.o AudioDecoderIec60958.o AudioDecoderMpeg1.o \
	AudioDecoderNull.o AudioDecoderPcm.o AudioOut.o \
	AudioPacketQueue.o AudioPlayer.o AudioPlayerBsp.o AudioPlayerHd.o \
	BspCommChannel.o BspOsd.o BspOsdProvider.o BkgPicPlayer.o \
	bspchannel.o bspshmlib.o dts.o fs453settings.o iec60958.o MpegPes.o \
	hdchannel.o hdshmlib.o HdCommChannel.o \
	Reel.o ReelBoxDevice.o ReelBoxMenu.o \
	VideoPlayer.o VideoPlayerBsp.o VideoPlayerHd.o \
	VideoPlayerPip.o VideoPlayerPipBsp.o VideoPlayerPipHd.o \
	VdrXineMpIf.o HdOsd.o HdOsdProvider.o HdTrueColorOsd.o HdFbTrueColorOsd.o setupmenu.o


### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

BSPSHM = ../../../utils/bspshm
HDSHM = ../../../utils/hdshm3/src

BSPINCLUDE = -I$(BSPSHM) -I$(BSPSHM)/include
HDINCLUDE = -I$(HDSHM) -I$(HDSHM)/include
LIBMAD     ?= ../../../../temp/docimage/libs/libmad
LIBASOUND  ?= ../../../../temp/docimage/libs/alsa-lib

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### Includes and Defines (add further entries here):

INCLUDES += -I$(LIBASOUND)/include $(BSPINCLUDE) $(HDINCLUDE)
INCLUDES += -I$(LIBMAD) `freetype-config --cflags`

ifdef REELSKIN
  DEFINES += -DREELSKIN
  OBJS += BspTrueColorOsd.o ReelSkin.o
  LIBS += -lpng
else
  DEFINES += -DNOT_THEME_LIKE
endif

DEFINES += -DPLAYER_VERSION=\"$(PLAYER_VERSION)\" -D__LINUX__ -D__STDC_CONSTANT_MACROS

ifdef REELVDR
  DEFINES += -DREELVDR
  LIBS += -lasound -lmad -lpng  -lavcodec -lswscale
else
  DEFINES += -DNOT_THEME_LIKE
  LIBS += -lasound -lmad -lpng  -lavcodec -lswscale
endif

LDFLAGS += -L$(LIBASOUND)/src/.libs
LDFLAGS += -L$(LIBMAD)/.libs

# Include common rules
#include $(VDRDIR)/Make.common

#useless-target-for-compatibility-with-vanilla-vdr:
#	$(LIBDIR)/$@.$(APIVERSION)
