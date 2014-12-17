#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
# IPORTANT: the presence of this macro is important for the Make.config
# file. So it must be defined, even if it is not used here!
#
PLUGIN = webbrowser

### The object files (add further files here):

OBJS = $(PLUGIN).o hdshmlib.o hdutil.o setup.o webbrowserosd.o \
	util.o menuPercItem.o

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp
HDSHMDIR = ../../../utils/hdshm3/src/include

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common


### Includes and Defines (add further entries here):

INCLUDES += -I$(HDSHMDIR)

DEFINES  += -DSYSCONFIGFNAME=\"$(SYSCONFIGFNAME)\"

