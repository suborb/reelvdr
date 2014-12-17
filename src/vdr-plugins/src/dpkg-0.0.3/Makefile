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
PLUGIN = dpkg
PLUGIN2 = dpkgopt

### The object files (add further files here):

#OBJS = $(PLUGIN).o i18n.o mainmenu.o
#OBJS2 = $(PLUGIN2).o i18n.o mainmenu.o
OBJS = $(PLUGIN).o mainmenu.o
OBJS2 = $(PLUGIN2).o mainmenu.o

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

LDFLAGS =

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common


### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

ifdef RBMINI
   TAGCOLL  := $(wildcard /usr/arm-linux-gnueabi/include/tagcoll-2.*)
   LIBAPT-FRONT-LIB = libapt-front-dev/lib-armel
else
   TAGCOLL  := $(wildcard /usr/include/tagcoll-2.*)
   LIBAPT-FRONT-LIB = libapt-front-dev/lib
endif

INCLUDES += -Isigc++-2.0
INCLUDES += -I$(TAGCOLL) -I.
#INCLUDES += -Ilibapt-front-dev/include

DEFINES  += -D__STL_CONFIG_H

#LIBS     += -L$(LIBAPT-FRONT-LIB) 
LIBS     += -lapt-front -lapt-pkg -ltagcoll2 -lwibble -lept

#menu.o dpkgitem.o pkgmenu.o cdmenu.o package.o dpkgthread.o

### Targets:

plug: plug2

all: libvdr-$(PLUGIN2).so i18n

plug2: libvdr-$(PLUGIN2).so

libvdr-$(PLUGIN2).so: $(OBJS2)
	$(CXX) $(CXXFLAGS) -shared $(OBJS2) $(LDFLAGS) $(LIBS) -o $@
	@cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)

clean: plugin-clean

plugin-clean:
	@-rm -f $(PODIR)/*.mo
	@-rm -f $(OBJS2) $(DEPFILE) *.so *.tgz core* *~


target-for-compatibility-with-vanilla-vdr:
	echo $(LIBDIR)/$@.$(APIVERSION)
