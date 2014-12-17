CC=gcc
CFLAGS=-O3
LDFLAGS=-s

ARCH= $(shell $(CC) -dumpmachine)
APPLE_DARWIN = $(shell echo $(ARCH) | grep -q 'apple-darwin' && echo "1" || echo "0")
CYGWIN = $(shell echo $(ARCH) | grep -q 'cygwin' && echo "1" || echo "0")
MIPSEL = $(shell echo $(ARCH) | grep -q 'mipsel' && echo "1" || echo "0")

ifeq ($(APPLE_DARWIN), 1)
DEFS:=$(DEFS) -I../common/darwin/include/ -DAPPLE
APPLE=1
CFLAGS:= $(CFLAGS) -fno-common -Wall
else
CFLAGS:= $(CFLAGS) -Wall
endif

ifeq ($(MIPSEL),1)
DEFS:=$(DEFS) -DMIPSEL
XML_LIB:=-lxml2
else
XML_INC:=`xml2-config --cflags`
XML_LIB:=`xml2-config --libs`
endif

INCLUDES:=$(INCLUDES) -I../client -I../common $(XML_INC)
DEFS:=$(DEFS) -g -D_FILE_OFFSET_BITS=64 -DCLIENT -D_GNU_SOURCE
LDADD:=$(LDADD) -L../client
STATICLIBS:=$(LIBS) ../client/libmcli.a -lfuse $(XML_LIB) -lpthread
LIBS:=$(LIBS) -lfuse $(XML_LIB) -lpthread -lmcli
LDFLAGS:=$(LDFLAGS) -Wl,--as-needed

dvbfuse_OBJECTS=main.o dvbfuse.o parse.o mclilink.o siparser.o crc32.o


all: dvbfuse dvbfuse-static

MAKEDEP = $(CC) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): makefile
	$(MAKEDEP) $(INCLUDES) $(dvbfuse_OBJECTS:%.o=%.c) > $@

-include $(DEPFILE)

dvbfuse: $(dvbfuse_OBJECTS) ../client/libmcli.so
	$(CC) $(LDFLAGS) $(dvbfuse_OBJECTS) $(LDADD) $(LIBS) -o $@

dvbfuse-static: $(dvbfuse_OBJECTS) ../client/libmcli.a
	$(CC) $(LDFLAGS) $(dvbfuse_OBJECTS) $(LDADD) $(STATICLIBS) -o $@

../client/libmcli.so: ../client/libmcli.a

../client/libmcli.a:
	make -C ../client

.c.o:
	$(CC) $(DEFS) $(INCLUDES)  $(CFLAGS) -c $<


clean:
	$(RM) $(DEPFILE) -f *.o *~ dvbfuse dvbfuse-static
