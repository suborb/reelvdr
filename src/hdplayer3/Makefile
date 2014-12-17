#SDK      = /home/reel/Arbeit/hde/cms-sdk-1.3.75/DECYPHER-SDK-1-3-0-RC1
#HDSHM    = /home/reel/Arbeit/svnw/rb-cs/src/hdshm

SDK      ?= /usr/src
HDSHM    ?= $(shell cd .. ; pwd)/utils/hdshm3/src
MIPSEL   ?= /opt/wischip/mipsel-linux-gnu/gcc-3.3.4-glibc-2.3.2

DISABLE_ERF = 1

CROSSCC     = mipsel-linux-gnu-gcc
#CROSSCFLAGS = -Wall -O0  -DCONFIG_MIPS -g3 -fno-omit-frame-pointer
CROSSCFLAGS = -Wall -O2  -DCONFIG_MIPS -g -fno-omit-frame-pointer

INCLUDES    = -I. -I${HDSHM} -I${HDSHM}/include -I${SDK}/linux-2.6.12.6-wis/include -I./inc
#-DDISABLE_RPC
LDFLAGS     = -lpthread
LDADD       =

export INCLUDES

MODULES = audio_parse.o channel.o hdplayer.o hdshmlib.o hdchannel.o \
	  player.o  reel.o demux.o  osd.o decoder.o  video_parse.o hw_specials.o

ifdef DISABLE_ERF
    DEFINES += -DDISABLE_ERF
else
    MODULES += erfplayer.o
    LDFLAGS += -L./lib -lnmtk -lz -ldecoder
endif

DEPFILE := .dependencies
MAKEDEP := $(CC) -MM -MG

PATH    += :$(MIPSEL)/bin
REEL_SHARE = /usr/share/reel
HDPLAYER_PATH := $(REEL_SHARE)/hdplayer

all: hdplayer ../../../precompiled/hdplayer.gz
#pc/hdtsplay

$(DEPFILE):
	$(MAKEDEP) $(INCLUDES) $(MODULES:%.o=%.c) > $@

-include $(DEPFILE)

hdplayer: $(MODULES)
	@rm -f $@
	$(CROSSCC) $(LDFLAGS) $(MODULES) $(LDADD) $(LIBS) -o $@
#	mipsel-linux-gnu-strip $@

.c.o:
	$(CROSSCC) $(DEFINES) $(INCLUDES) $(CROSSCFLAGS) -c $<

hdplayer-hostapp: pc/hdtsplay

hdplayer-hostapp-clean:
	-$(MAKE) -C pc clean

pc/hdtsplay:
	$(MAKE) -C $(@D) $(@F)

../../../precompiled/hdplayer.gz: hdplayer
	gzip -c $< > $@

clean:
	$(RM) *.o *~ hdplayer $(DEPFILE)

install: hdplayer
	sudo mv $(HDPLAYER_PATH)/hdplayer $(HDPLAYER_PATH)/hdplayer.old
	sudo install -p hdplayer $(HDPLAYER_PATH)
	/bin/echo -e "hdplayermgr -g\r\nexit\n" | nc -w 30 hdext 23
