#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.1.1.1 2004/11/19 16:45:31 lordjaxom Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = osdpip

### The object files (add further files here):

OBJS = $(PLUGIN).o osd_info.o osd.o receiver.o player.o setup.o pesassembler.o PreviewChannel.o

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common


### Defines

ifdef FFMPEG_STATIC
	DEFINES += -DHAVE_FFMPEG_STATIC
endif

### Implicit rules:


### Dependencies:


### Targets:

