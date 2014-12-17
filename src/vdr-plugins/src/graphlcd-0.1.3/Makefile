#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = graphlcd

### The object files (add further files here):

OBJS = display.o global.o logo.o logolist.o layout.o menu.o plugin.o setup.o state.o strfct.o widgets.o

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -g -Wall -Woverloaded-virtual

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp


### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common


### Includes and Defines (add further entries here):

LIBGLCD ?= ../../../libs/graphlcd-base
export INSTALLPREFIX = $(LIBGLCD)/glcdgraphics
export INSTALLPREFIXDRIVERS = $(LIBGLCD)/glcddrivers

INCLUDES += -I$(LIBGLCD)

#ifdef HAVE_FREETYPE2
    DEFINES += -DHAVE_FREETYPE2
    ifdef RBMINI
        INCLUDES += -I/usr/arm-linux-gnueabi/include/freetype2
    else
        INCLUDES += -I/usr/include/freetype2
    endif
#endif

LIBS += -L$(INSTALLPREFIX)  -L$(INSTALLPREFIXDRIVERS) \
	-shared -lglcddrivers -lglcdgraphics -lstdc++

