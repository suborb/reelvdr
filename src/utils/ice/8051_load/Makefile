CC       ?= gcc
CFLAGS   ?= -O2 -Wall -m32
LDFLAGS  ?= -O6 -m32 


DEFINES  += -D__LINUX__ 
CFLAGS += -Wno-unused-result
INCLUDES  = 

load_8051_OBJECTS= load_8051.o

all: load_8051

.c.o:
	$(CC) $(DEFINES) $(INCLUDES) $(CFLAGS) -c $<

# dependencies
MAKEDEP = $(CC) -MM -MG 
DEPFILE = .dependencies
$(DEPFILE): Makefile
	$(MAKEDEP) $(DEFINES) $(INCLUDES) *.c > $@

-include $(DEPFILE)

load_8051: $(load_8051_OBJECTS)
	$(CC)  $(LDFLAGS) $(LD_EXTRA_FLAGS) $(load_8051_OBJECTS) $(LDADD) $(LIBS) -o $@
	strip $@

clean:
	$(RM)  *.o *~ load_8051 $(DEPFILE)
