#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN  = mediaplayer
PLUGIN2 = mediaplayerdvd
PLUGIN3 = audioplayer
PLUGIN4 = videoplayer
PLUGIN5 = imageplayer
PLUGIN6 = videoplayer2
PLUGIN7 = audioplayer2

### The object files (add further files here):

OBJS = $(PLUGIN).o playermenu.o playerStatus.o
OBJS2 = $(PLUGIN2).o
OBJS3 = $(PLUGIN3).o
OBJS4 = $(PLUGIN4).o
OBJS5 = $(PLUGIN5).o
OBJS6 = $(PLUGIN6).o
OBJS7 = $(PLUGIN7).o

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
include $(VDRDIR)/Make.common

### Includes and Defines (add further entries here):

DEFINES  += -D__STL_CONFIG_H
DEFINES2 += $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN2)"'
DEFINES3 += $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN3)"'
DEFINES4 += $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN4)"'
DEFINES5 += $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN5)"'
DEFINES6 += $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN6)"'
DEFINES7 += $(DEFINES) -DPLUGIN_NAME_I18N='"$(PLUGIN7)"'

### Implicit rules:

mediaplayerdvd.o: mediaplayerdvd.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES2) $(INCLUDES) $<

audioplayer.o: audioplayer.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES3) $(INCLUDES) $<

videoplayer.o: videoplayer.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES4) $(INCLUDES) $<

imageplayer.o: imageplayer.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES5) $(INCLUDES) $<

audioplayer2.o: audioplayer2.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES6) $(INCLUDES) $<

videoplayer2.o: videoplayer2.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES7) $(INCLUDES) $<

### Targets:
plug: plug2 plug3 plug4 plug5 plug6 plug7
all: plug i18n

plug2: libvdr-$(PLUGIN2).so
plug3: libvdr-$(PLUGIN3).so
plug4: libvdr-$(PLUGIN4).so
plug5: libvdr-$(PLUGIN5).so
plug6: libvdr-$(PLUGIN6).so
plug7: libvdr-$(PLUGIN7).so

$(I18Npot): $(wildcard *.h)

libvdr-$(PLUGIN2).so: $(OBJS2)
	$(CXX) $(CXXFLAGS) -shared $(OBJS2) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

libvdr-$(PLUGIN3).so: $(OBJS3)
	$(CXX) $(CXXFLAGS) -shared $(OBJS3) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

libvdr-$(PLUGIN4).so: $(OBJS4)
	$(CXX) $(CXXFLAGS) -shared $(OBJS4) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

libvdr-$(PLUGIN5).so: $(OBJS5)
	$(CXX) $(CXXFLAGS) -shared $(OBJS5) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

libvdr-$(PLUGIN6).so: $(OBJS6)
	$(CXX) $(CXXFLAGS) -shared $(OBJS6) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

libvdr-$(PLUGIN7).so: $(OBJS7)
	$(CXX) $(CXXFLAGS) -shared $(OBJS7) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

clean: clean-plug2 clean-plug3 clean-plug4 clean-plug5 clean-plug6 clean-plug7
clean-plug2:
	@rm -f $(OBJS2)
	@-rm -f $(LIBDIR)/libvdr-$(PLUGIN2).so.*

clean-plug3:
	@rm -f $(OBJS3)
	@-rm -f $(LIBDIR)/libvdr-$(PLUGIN3).so.*

clean-plug4:
	@rm -f $(OBJS4)
	@-rm -f $(LIBDIR)/libvdr-$(PLUGIN4).so.*

clean-plug5:
	@rm -f $(OBJS5)
	@-rm -f $(LIBDIR)/libvdr-$(PLUGIN5).so.*

clean-plug6:
	@rm -f $(OBJS6)
	@-rm -f $(LIBDIR)/libvdr-$(PLUGIN6).so.*

clean-plug7:
	@rm -f $(OBJS7)
	@-rm -f $(LIBDIR)/libvdr-$(PLUGIN7).so.*

target-for-compatibility-with-vanilla-vdr:
	$(LIBDIR)/$@.$(APIVERSION)
