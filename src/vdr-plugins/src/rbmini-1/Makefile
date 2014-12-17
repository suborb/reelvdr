# Makefile for the ReelBox Mini output plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = rbmini

### The object files (add further files here):

OBJS = $(PLUGIN).o rbm_device.o rbm_osd.o rbm_setup.o rbm_hdosd.o

EXTRA_I18N += include/cnxt_rbm.h

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

### Includes and Defines (add further entries here):

#INCLUDES += -I$(CNXT_DIR)/src/include

INCLUDES += -Iinclude 

ifdef RBMINI
    INCLUDES += -I/usr/arm-linux-gnueabi/include/freetype2
else
    INCLUDES += `freetype-config --cflags`
endif

LIBS += -lpng
