CROSS    =
CC      ?= $(CROSS)gcc
CFLAGS  ?=  -O2 -Wall
LDFLAGS ?=


DEFINES  += -D__LINUX__ 
INCLUDES  = -I../../src/include
INCLUDES += -I../../src/driver

hdtest_OBJECTS = hdtest.o hdshmlib.o hdchannel.o

all: hdtest

.c.o:
	$(CC) $(DEFINES) $(INCLUDES) $(CCFLAGS) -c $<

# dependencies
MAKEDEP = $(CC) -MM -MG 
DEPFILE = .dependencies
$(DEPFILE): Makefile
	$(MAKEDEP) $(DEFINES) $(INCLUDES) *.c > $@

-include $(DEPFILE)

hdtest: $(hdtest_OBJECTS)
	@rm -f hdtest
	$(CC) $(LDFLAGS) $(LD_EXTRA_FLAGS) $(hdtest_OBJECTS) $(LDADD) $(LIBS) -o $@

clean:
	$(RM)  *.o *~ hdtest $(DEPFILE)
