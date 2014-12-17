LDFLAGS = -m32 
CFLAGS = -m32 -O3
INCLUDES = 

rtl_prog_OBJECTS= memsettings.o

all: memsettings

memsettings: $(rtl_prog_OBJECTS)
	@rm -f $@
	$(CC) $(LDFLAGS) $(rtl_prog_OBJECTS) $(LDADD) $(LIBS) -o $@
	strip $@
.c.o:
	$(CC) $(DEFINES) $(INCLUDES)  $(CFLAGS) -c $<

clean:
	$(RM)  *.o *~