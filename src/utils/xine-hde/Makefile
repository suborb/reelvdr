PROJ_NAME = xineplug_hde
-include defines.mak

all: $(PROJ_NAME).so

TEMP_DIR  = ../../../temp
#LINUX_DIR = /usr/include
LINUX_DIR = /usr/src/linux-2.6.12-rc2-wis/include
#LINUX_DIR = $(TEMP_DIR)/cdimage/flash/kernel-2.6.11.11/include
#XINE_DIR  = $(TEMP_DIR)/docimage/libs/xine/include
XINE_DIR  ?= /usr/include/xine
XINE_DIR1  ?= /usr/local/include/xine
#HDSHM_DIR  = /usr/src/reel/testing/src/utils/hdshm3/src/include
HDSHM_DIR  = ../hdshm3/src/include
HDSHMLIB   = ../hdshm3/src/hostlib
HDCHANNEL  = ../hdshm3/src/hdchannel

### C++ compiler and options:

CC       ?= gcc
CXX      ?= g++
#CFLAGS   ?= -g3 -O0 -Wall
#CXXFLAGS ?= -g3 -O0 -Wall

# CFLAGS   ?= -O2 -Wall
CFLAGS   ?= -O2 -funroll-loops -fomit-frame-pointer -march=pentium-mmx -mmmx -Wall -Wstrict-prototypes
CXXFLAGS ?= -O2 -Wall

### Includes and Defines:
INCLUDES = -I$(XINE_DIR) -I$(XINE_DIR)/xine -I$(HDSHM_DIR) -I$(LINUX_DIR) -I$(HDSHMLIB) -I$(HDCHANNEL)

DEFINES = -D_GNU_SOURCE -D__LINUX__ $(HDE_PLAYER)

DEFINES += -DCONFIG_MPEGVIDEO_PARSER
DEFINES += -DCONFIG_MPEG4VIDEO_PARSER
DEFINES += -DCONFIG_H264_PARSER

COBJS = hde_xine.o hde_xine_video.o hde_xine_audio.o hde_xine_vd.o hde_xine_ad.o hde_xine_sd.o \
        hde_xine_scr.o hde_io.o hde_cmds.o hde_tool.o parser.o mpeg12.o mpeg4.o h264.o golomb.o

CPPOBJS = 

## Implicit rules:

%.o: %.c
	$(CC) $(CFLAGS) -c $(DEFINES) $(INCLUDES) $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@echo creating dependencies for pkg $(PROJ_NAME)
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(COBJS:%.o=%.c) $(COBJS:%.o=%.c) $(CPPOBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Targets:

$(PROJ_NAME).so: $(COBJS) $(CPPOBJS) 
	$(CXX) $(CFLAGS) -shared $(COBJS) $(CPPOBJS) $(LDFLAGS) $(LIBS) -o $@

clean:
	@-rm -f $(COBJS) $(CPPOBJS) $(DEPFILE) *.so *.tgz core* *~

install: all
	cp -av $(PROJ_NAME).so `xine-config --plugindir`

#LIBXINE ?=../../../temp/docimage/libs/xine-lib/src/xine-engine
LIBXINE ?=/usr/lib

XPLAYEROBJS = xplayer.o
XPLAYERLDFLAGS = -L$(LIBXINE)/.libs
XPLAYERLIBS += -lxine
 
xplayer: $(XPLAYEROBJS)
	$(CXX) $(CXXFLAGS) $(XPLAYEROBJS) $(XPLAYERLDFLAGS) $(XPLAYERLIBS) -o $@ 
