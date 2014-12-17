#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = bgprocess

### The object files (add further files here):

OBJS = $(PLUGIN).o 


### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -fPIC -g -O2 -Wall -Woverloaded-virtual

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common


### Includes and Defines (add further entries here):

INCLUDES +=

DEFINES +=

### Aditional rules
i18n.c: i18n-template.c po2i18n.pl $(I18Npo)
	./po2i18n.pl < $< > $@

clean: clean-plugin
clean-plugin:
	@-rm -f i18n.c
