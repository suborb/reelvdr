#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = install

### The object files (add further files here):

OBJS = $(PLUGIN).o ipnumitem.o installmenu.o util.o menueditrefreshitems.o \
	install_language.o install_connections.o install_diseqc.o \
	install_client.o install_channellist.o install_channelscan.o install_network.o \
	install_update.o install_formathd.o install_video.o install_video_mini.o netinfo.o \
	install_video_ice.o install_multiroom.o search_netceiver.o system_devices.o \
        sysconfig_backup.o tools.o disk_tools.o

EXTRA_I18N += ../rbmini/include/cnxt_rbm.h

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp
BSPLIBDIR = ../../../utils/bspshm

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config
-include $(VDRDIR)/Make.common

### Includes and Defines (add further entries here):

INCLUDES += -I../../$(DVBDIR)/include -I$(BSPLIBDIR)/include

DEFINES += -DCONFIGDIR=\"$(CONFIGDIR)\" -DSYSCONFIGFNAME=\"$(SYSCONFIGFNAME)\"

LIBS += -lavahi-client -ludev
ifdef RBMINI
  LIBS += -lparted
else
  LIBS += `pkg-config libparted --libs`
endif
