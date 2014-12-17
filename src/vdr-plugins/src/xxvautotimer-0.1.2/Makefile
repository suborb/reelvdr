#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.9 2006/08/15 20:45:24 ralf Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = xxvautotimer
MYSQLLIBDIR = /usr/lib
MYSQLWRAPPERDIR=./mysqlwrapped-1.4
MYSQLWRAPPERLIB=libmysqlwrapped.a


### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).cpp | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:
CXX      ?= g++
CXXFLAGS ?= -fPIC -O2 -Wall -Woverloaded-virtual -g

### The directory environment:

#DVBDIR = ../../../../DVB
VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR (taken from VDR's "config.h"):

VDRVERSION = $(shell grep 'define VDRVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include -I$(DVBDIR)/include -I$(MYSQLWRAPPERDIR)

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### Test wareagle-patch

ifeq ($(shell test -f $(VDRDIR)/iconpatch.h ; echo $$?),0)
  DEFINES += -DHAVE_ICONPATCH
endif

DEFINES +=-DDEBUGXXVAUTOTIMER


### The object files (add further files here):

OBJS = $(PLUGIN).o setup.o menu.o i18n.o autotimer.o inifile.o mysocket.o

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Targets:

all: $(MYSQLWRAPPERDIR)/$(MYSQLWRAPPERLIB) libvdr-$(PLUGIN).so  


$(MYSQLWRAPPERDIR)/$(MYSQLWRAPPERLIB):
	cd $(MYSQLWRAPPERDIR);make all


libvdr-$(PLUGIN).so: $(OBJS) $(MYSQLWRAPPERDIR)/$(MYSQLWRAPPERLIB)
	$(CXX) $(CXXFLAGS) -L$(MYSQLLIBDIR) -lmysqlclient -lz -shared  $(OBJS) $(MYSQLWRAPPERDIR)/$(MYSQLWRAPPERLIB) -o $@ 
	@cp $@ $(LIBDIR)/$@.$(VDRVERSION)

dist: clean
	@rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@rm -f $(TMPDIR)/$(ARCHIVE)/xxvautotimer.kdevelop
	@rm -f $(TMPDIR)/$(ARCHIVE)/xxvautotimer.kdevelop.filelist
	@rm -f $(TMPDIR)/$(ARCHIVE)/xxvautotimer.kdevelop.pcs
	@rm -f $(TMPDIR)/$(ARCHIVE)/xxvautotimer.kdevses
	@rm -rf $(TMPDIR)/$(ARCHIVE)/CVS
	@rm -rf $(TMPDIR)/$(ARCHIVE)/Scripte/CVS
	@rm -rf $(TMPDIR)/$(ARCHIVE)/mysqlwrapped-1.4/CVS
	@ln -s $(ARCHIVE) $(TMPDIR)/$(PLUGIN)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE) $(PLUGIN)
	@-rm -rf $(TMPDIR)/$(ARCHIVE) $(TMPDIR)/$(PLUGIN)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@ cd $(MYSQLWRAPPERDIR);make clean
	@rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
