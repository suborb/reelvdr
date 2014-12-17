#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = osdteletext

### The object files (add further files here):

OBJS = $(PLUGIN).o menu.o txtfont.o txtrecv.o txtrender.o displaybase.o display.o


### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

include $(VDRDIR)/Make.config

### Make sure that necessary options are included (VDR >= 1.7.13):

include $(VDRDIR)/Make.common

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"' -D_GNU_SOURCE


### Implicit rules:
