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
PLUGIN = netcv
PLUGIN4 = netcvupdate

### The object files (add further files here):

OBJS = $(PLUGIN).o diseqcsetting.o diseqcsettingsimple.o diseqcsettingitem.o \
	netcvcam.o netcvdevice.o netcvdiag.o netcvinstallmenu.o \
	netcvinfomenu.o netcvmenu.o netcvthread.o netcvupdatemenu.o netcvtuner.o
OBJS4 = $(PLUGIN4).o netcvupdatemenu.o netcvthread.o


### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common

### Includes and Defines (add further entries here):

INCLUDES   += -I$(VDRDIR)/$(PLUGINDIR)/src/mcli/
LIBS       += -L$(VDRDIR)/$(PLUGINDIR)/src/mcli/mcast/client -lmcli

ifdef RBMINI
  INCLUDES += -I/usr/arm-linux-gnueabi/include/libxml2
  LIBS     += -lxml2
else
  INCLUDES += `xml2-config --cflags`
  LIBS     += `xml2-config --libs`
endif

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'




### Implicit rules:

netcvupdate.o: netcvupdate.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN4)"' $(INCLUDES) $<

$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES1) $(INCLUDES4) $(OBJS:%.o=%.c) $(OBJS4:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

I18Nmsgs1  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Nmsgs4  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN4).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))

#$(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN4).mo: $(PODIR)/%.po
#	@echo "making $@"
#	@mkdir -p $(dir $@)
#	msgfmt -c -o $@ $<

$(I18Nmsgs4): $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN4).mo: $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo
	@mkdir -p $(dir $@)
	cp $< $@

#I18Npot   += $(PODIR)/$(PLUGIN4).pot

.PHONY: i18n
i18n: $(I18Nmsgs4)
i18n-dist: $(I18Nmsgs4)

### Targets:

plug: libvdr-$(PLUGIN4).so
all: plug


libvdr-$(PLUGIN4).so: $(OBJS4)
	$(CXX) $(CXXFLAGS) -shared $(OBJS4) $(LIBS) -o $@
	@cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)

clean: clean-plugs
clean-plugs:
	@-rm -f $(OBJS) $(OBJS4) $(DEPFILE) *.so *.tgz core* *~
