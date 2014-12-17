#
# Makefile for the reelblog plugin. A special version of the RSS Reader Plugin (Version 1.0.2).
#
#  Copyright (C) 2008 Heiko Westphal

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = reelblog

### The object files (add further files here):

OBJS = reelblog.o parser.o menu.o tools.o

# Debugging on/off 
# REELBLOG_DEBUG = 0

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Libraries

LIBS = -lexpat `curl-config --libs`

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

ifdef REELBLOG_DEBUG
DEFINES += -DDEBUG
endif

INCLUDES += `curl-config --cflags`

# Strip debug symbols?  Set eg. to /bin/true if not
STRIP = strip

