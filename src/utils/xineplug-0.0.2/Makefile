PROJ_NAME      = xineplug_reelbox
XINE_INCLUDES ?= ../xine-include/
XINE_SRC      ?= ../xine-lib


all: $(PROJ_NAME).so

### C++ compiler and options:

CC       ?= gcc
CXX      ?= g++
CFLAGS   ?= -O2 -Wall
CXXFLAGS ?= -O2 -Wall

### Includes and Defines:
#INCLUDES = -I$(XINE_INCLUDES) -I$(XINE_INCLUDES)/xine -I$(XINE_INCLUDES)/xine-utils
INCLUDES = -I$(XINE_SRC)/src -I$(XINE_SRC)/include -I$(XINE_SRC)/src/xine-utils \
           -I$(XINE_SRC)/src/xine-engine -I$(XINE_SRC)/lib -I$(XINE_SRC)/src/input/libdvdnav/

## -I$(XINE_SRC)/xine-engine -I$(XINE_SRC)/xine-utils
DEFINES = -D_GNU_SOURCE -D__LINUX__ -DXINE_COMPILE

COBJS = audio_out.o mpeg_videodec.o spu.o spu_If.o xine_decoder.o plugin.o

CPPOBJS = ReadBuffer.o VideoStreamParser.o

## Implicit rules:

%.o: %.c
	$(CC) $(CFLAGS) -c $(DEFINES) $(INCLUDES) $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	$(MAKEDEP) $(DEFINES) $(INCLUDES) $(COBJS:%.o=%.c) $(COBJS:%.o=%.c) $(CPPOBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Targets:

$(PROJ_NAME).so: $(COBJS) $(CPPOBJS)
	$(CXX) $(CFLAGS) -shared $(COBJS) $(CPPOBJS) $(LDFLAGS) $(LIBS) -o $@

clean:
	@-rm -f $(COBJS) $(CPPOBJS) $(DEPFILE) *.so *.tgz core* *~

install: all
	@cp $(PROJ_NAME).so /home/gerald/xine/plugins
