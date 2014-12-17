#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# VDR-Patches:
# comment the following out, 
# if you have patched vdr with the additional patches,
# see plugin-subdirectory patches
### for device-patch, not needed @ vdr >= 1.3.45 !!!
### for closemanu-patch, not use @ vdr >= 1.3.47 !!!
#VDRP_DTRANSFER = 1
### for closemenu-patch
#VDRP_CLOSEMENU = 1


# WARNING!!! DONT turn this option off! At moment you will need Stillpicture!
# If your Primary Device doesn't support Stillpicture, you can deactivate it here.
# (Don't set it to 0, use a # ;-)
USE_STILLPICTURE = 1

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = radio

### The object files (add further files here):

OBJS = $(PLUGIN).o radioaudio.o radioskin.o radiotools.o i18n.o

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

#DVBDIR = ../../../../DVB
VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

# need for vdr <= 1.3.47
ifeq ($(strip $(APIVERSION)),)
    APIVERSION = $(shell grep 'define VDRVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')
endif

ifdef USE_STILLPICTURE 
DEFINES += -DUSE_STILLPICTURE
endif

ifdef VDRP_DTRANSFER
DEFINES += -DVDRP_DTRANSFER
endif

ifdef VDRP_CLOSEMENU
DEFINES += -DVDRP_CLOSEMENU
endif

