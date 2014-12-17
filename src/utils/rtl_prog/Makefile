LDFLAGS = -m32 
CFLAGS = -m32 -O3
INCLUDES = 

rtl_prog_OBJECTS= rtl_prog.o

all: rtl_prog

rtl_prog: $(rtl_prog_OBJECTS)
	@rm -f $@
	$(CC) $(LDFLAGS) $(rtl_prog_OBJECTS) $(LDADD) $(LIBS) -o $@
	strip $@
.c.o:
	$(CC) $(DEFINES) $(INCLUDES)  $(CFLAGS) -c $<

clean:
	$(RM)  *.o *~ rtl_prog
