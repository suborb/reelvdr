LDFLAGS = -m32 -lpthread
CFLAGS =  -m32 -O2
INCLUDES = 
DEFINES += -DLIRC_DEVICE=\"$(LIRC_DEVICE)\"
DEFINES += -DFP_DEVICE=\"$(FP_DEVICE)\"

vdr_control_OBJECTS= vdr_control.o frontpanel.o

all: vdr_control

vdr_control: $(vdr_control_OBJECTS)
	$(CC) $(LDFLAGS) $(vdr_control_OBJECTS) $(LDADD) $(LIBS) -o $@
	strip $@

.c.o:
	$(CC) $(DEFINES) $(INCLUDES)  $(CFLAGS) -c $<

clean:
	$(RM)  *.o *~ vdr_control
