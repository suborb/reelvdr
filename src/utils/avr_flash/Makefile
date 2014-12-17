LDFLAGS ?= -m32 
CFLAGS  ?= -m32 -O3
INCLUDES = 
DEFINES += -DFP_DEVICE=\"$(FP_DEVICE)\"
include ../../Make.config

all: avr_flasher avr_flasher1

avr_flasher: flasher.c
	@rm -f $@
	$(CC) $(DEFINES) $(INCLUDES)  $(CFLAGS) -c flasher.c
	$(CC) $(LDFLAGS) flasher.o $(LDADD) $(LIBS) -o $@
	strip $@

avr_flasher1: flasher.c
	@rm -f $@
	$(CC) $(DEFINES) -DNOFWREAD $(INCLUDES)  $(CFLAGS) -c flasher.c
	$(CC) $(LDFLAGS) flasher.o $(LDADD) $(LIBS) -o $@
	strip $@

clean:
	$(RM)  *.o *~ avr_flasher avr_flasher1
