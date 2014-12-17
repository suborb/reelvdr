#SDK      = /home/reel/Arbeit/hde/cms-sdk-1.3.75/DECYPHER-SDK-1-3-0-RC1
#HDSHM    = /home/reel/Arbeit/svnw/rb-cs/src/hdshm

SDK      ?= /usr/src
HDSHM    ?= $(shell cd .. ; pwd)/utils/hdshm/src
MIPSEL   ?= /opt/wischip/mipsel-linux-gnu/gcc-3.3.4-glibc-2.3.2

CROSSCC     = mipsel-linux-gnu-gcc
CROSSCFLAGS = -Wall -O3 -funroll-loops -DCONFIG_MIPS

INCLUDES    = -I. -I${HDSHM} -I${HDSHM}/include -I${SDK}/linux-2.6.12.6-wis/include -I./inc
LDFLAGS     = -lpthread -L./lib -lnmtk -lz -ldecoder
LDADD       =

export INCLUDES

MODULES = ac3_parser.o audio_parser.o channel.o hdplayer.o hdshmlib.o hdchannel.o \
	mpa_parser.o player.o player_ll.o reel.o ts_demux.o vfb.o osd.o erfplayer.o

DEPFILE := .dependencies
MAKEDEP := $(CC) -MM -MG

PATH    += :$(MIPSEL)/bin

all: 
	@echo
	@echo "   This version of hdplayer is obsolete. Type"
	@echo "   \"make i-know-what-im-doing\""
	@echo "   to compile it anyway."
	@echo

i-know-what-im-doing: hdplayer pc/hdtsplay ../../../precompiled/hdplayer.gz

$(DEPFILE):
	$(MAKEDEP) $(INCLUDES) $(MODULES:%.o=%.c) > $@

-include $(DEPFILE)

hdplayer: $(MODULES)
	@rm -f $@
	$(CROSSCC) $(LDFLAGS) $(MODULES) $(LDADD) $(LIBS) -o $@
	mipsel-linux-gnu-strip $@

.c.o:
	$(CROSSCC) $(DEFINES) $(INCLUDES) $(CROSSCFLAGS) -c $<

hdplayer-hostapp: pc/hdtsplay

hdplayer-hostapp-clean:
	$(MAKE) -C pc clean

pc/hdtsplay:
	$(MAKE) -C $(@D) $(@F)

../../../precompiled/hdplayer.gz: hdplayer
	gzip -c $< > $@

clean:
	$(RM) *.o *~ hdplayer $(DEPFILE)

