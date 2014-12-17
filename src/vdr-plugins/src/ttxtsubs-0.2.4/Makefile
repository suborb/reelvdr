#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.22 2004/05/31 19:26:05 ragge Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = ttxtsubs

### The object files (add further files here):
OBJS = $(PLUGIN).o ttxtsubsdisplayer.o ttxtsubsdisplay.o teletext.o \
	ttxtsubspagemenu.o ttxtsubschannelsettings.o ttxtsubslivereceiver.o

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib

### Allow user defined options to overwrite defaults:

include $(VDRDIR)/Make.config

### Make sure that necessary options are included (VDR >= 1.7.13):

include $(VDRDIR)/Make.common

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)
BUPPACKAGE = vdr-$(ARCHIVE)-bup-$(shell date +%y%m%d-%H%M%S)

### Includes and Defines (add further entries here):

### Implicit rules:
