#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.6 2006/06/18 11:46:31 schmitzj Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = timeline

### The object files (add further files here):

OBJS = $(PLUGIN).o i18n.o checkerOsd.o config.o

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

