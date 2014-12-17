CROSS=mipsel-linux-gnu-
CC=$(CROSS)gcc
LDFLAGS =

CFLAGS=  -O2 -Wall
DEFINES = -D__LINUX__ -DCONFIG_MIPS
INCLUDES = -I../../src/include

hdtest_OBJECTS= hdtest.o hdshmlib.o hdchannel.o

all: hdtest

.c.o:
	$(CC) $(DEFINES) $(INCLUDES) $(CFLAGS) -c $<

# dependencies
MAKEDEP = $(CC) -MM -MG 
DEPFILE = .dependencies
$(DEPFILE): Makefile
	$(MAKEDEP) $(DEFINES) $(INCLUDES) *.c > $@

-include $(DEPFILE)

hdtest: $(hdtest_OBJECTS)
	@rm -f hdtest
	$(CC) $(LDFLAGS) $(LD_EXTRA_FLAGS) $(hdtest_OBJECTS) $(LDADD) $(LIBS) -o $@
	$(CROSS)strip $@

clean:
	$(RM)  *.o *~ hdtest $(DEPFILE)
