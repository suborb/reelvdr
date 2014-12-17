#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = bouquets

### The object files (add further files here):

OBJS = $(PLUGIN).o MenuBouquets.o MenuBouquetsList.o \
	MenuEditBouquet.o SetChannelOptionsMenu.o MenuBouquetItem.o \
	MenuChannelItem.o MenuEditChannel.o
#filetools.o i18n.o

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

