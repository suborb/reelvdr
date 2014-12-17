#
# Makefile for a Video Disk Recorder plugin
#

# uncomment if you use the pin-plugin and want to protect your recordings
WITHPINPLUGIN=1
WITHOWNUNDELETE=1

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = extrecmenu
PLUGIN2 = undelete


ALL = libvdr-$(PLUGIN).so
ifdef  WITHOWNUNDELETE
  ALL += libvdr-$(PLUGIN2).so
endif


### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config


### Includes and Defines (add further entries here):

DEFINES2 += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN2)"'

ifdef WITHPINPLUGIN
  DEFINES += -DWITHPINPLUGIN
endif


### The object files (add further files here):

ifdef REELVDR
OBJS = $(PLUGIN).o mymenurecordings.o mymenuundelete.o myreplaycontrol.o \
       mymenusetup.o mymenucommands.o tools.o mymenueditrecording.o SelectFile.o \
       ExternalMedia.o SelectMedia.o du.o MenuSearchRecordings.o
else
OBJS = $(PLUGIN).o mymenurecordings.o mymenuundelete.o myreplaycontrol.o \
       mymenusetup.o mymenucommands.o patchfont.o tools.o mymenueditrecording.o
endif

OBJS2    = undelete.o


### Common Makefile rules

-include $(VDRDIR)/Make.common

### Targets:
plug: libvdr-$(PLUGIN2).so

all: plug

$(I18Npot): mymenucommands.h

### Implicit rules:


# Dependencies:

$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) $(OBJS2:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):


libvdr-$(PLUGIN2).so: $(OBJS2)
	$(CXX) $(CXXFLAGS) -shared $(OBJS2) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)


clean: clean-plug2
clean-plug2:
	@-rm -f $(OBJS) $(OBJS2) $(DEPFILE) *.so *.tgz core* *~

target-for-compatibility-with-vanilla-vdr:
	$(LIBDIR)/$@.$(APIVERSION)
