#
# Makefile for the Video Disk Recorder
#
# See the main source file 'vdr.c' for copyright information and
# how to reach the author.
#
# $Id: Makefile 1.95 2006/08/20 10:44:22 kls Exp $

.DELETE_ON_ERROR:

CC       ?= ccache gcc
CFLAGS   ?= -g -O2 -Wall

CXX      ?= ccache g++
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual


include Make.config

VDRDIR = vdr-1.4

all: $(VDRDIR)/vdr plugins
vdr: $(VDRDIR)/vdr
vdr-plugins: plugins

clean: vdr-clean dvb-clean bspshm-clean hdshm-clean


$(VDRDIR)/vdr:
	$(MAKE) -C $(@D) vdr

plugins:
	$(MAKE) -C $(VDRDIR) plugins

vdr-clean:
	$(MAKE) -C $(VDRDIR) clean

dvb-clean:
	$(MAKE) -C kernel/dvb-kernel clean
	$(MAKE) -C kernel/reeldvb clean

bspshm-clean:
	$(MAKE) -C utils/bspshm clean

hdshm-clean:
	$(MAKE) -C utils/hdshm3 clean

hdplayer-hostapp:
	$(MAKE) -C hdplayer3 hdplayer-hostapp
