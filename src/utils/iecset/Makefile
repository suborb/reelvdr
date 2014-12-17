LDFLAGS = -m32 
CFLAGS ?= -m32 -O3
INCLUDES = -I../../libs/alsa-lib/include
LDFLAGS = -L../../libs/alsa-lib/src/.libs
LIBS = -lasound

OBJECTS= iecbits.o iecset.o 

all: iecset

.c.o:
	$(CC) $(DEFINES) $(INCLUDES)  $(CFLAGS) -c $<

iecset: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) $(LIBS) -o $@
	strip $@

clean:
	$(RM)  *.o *~ iecset
