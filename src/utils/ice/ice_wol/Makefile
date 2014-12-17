CC       ?= gcc
CFLAGS   ?= -O2 -Wall -m32
LDFLAGS  ?= -O6 -m32 


DEFINES  += -D__LINUX__ 
CFLAGS   += -Wno-unused-result
INCLUDES  = 

ice_wol_OBJECTS= ice_wol.o

all: ice_wol

.c.o:
	$(CC) $(DEFINES) $(INCLUDES) $(CFLAGS) -c $<

# dependencies
MAKEDEP = $(CC) -MM -MG 
DEPFILE = .dependencies
$(DEPFILE): Makefile
	$(MAKEDEP) $(DEFINES) $(INCLUDES) *.c > $@

-include $(DEPFILE)

ice_wol: $(ice_wol_OBJECTS)
	$(CC)  $(LDFLAGS) $(LD_EXTRA_FLAGS) $(ice_wol_OBJECTS) $(LDADD) $(LIBS) -o $@
	strip $@

clean:
	$(RM)  *.o *~ ice_wol $(DEPFILE)
