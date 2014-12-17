#SDK      = /home/acher/exp/reelbox/cms/DECYPHER-SDK-1-4-2-RC3/
#HDSHM    = /home/acher/exp/reelbox/rb-cs/src/utils/hdshm/src

CC        ?= gcc
CFLAGS    ?= -Wall -O3 -funroll-loops -m32
#INCLUDES  = -I. -I${HDSHM} -I${HDSHM}/include 
#LDFLAGS   = -lpthread -m32
#LDADD     =

MODULES = hdtsplay.o hdshmlib.o hdchannel.o

DEPFILE := .dependencies
MAKEDEP := $(CC) -MM -MG

PATH    += :$(MIPSEL)/bin

all: hdtsplay-helper

$(DEPFILE):
	$(MAKEDEP) $(INCLUDES) $(MODULES:%.o=%.c) > $@

-include $(DEPFILE)

hdtsplay-helper:
	$(MAKE) -C .. hdplayer-hostapp


hdtsplay: $(MODULES)
	@rm -f $@
	$(CC) $(LDFLAGS) $(MODULES) $(LDADD) $(LIBS) -o $@

.c.o:
	$(CC) $(DEFINES) $(INCLUDES) $(CFLAGS) -c $<

clean:
	$(RM) *.o *~ hdtsplay $(DEPFILE)
