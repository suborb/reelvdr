
COPTFLAGS := -O3 
CC ?= gcc
CFLAGS ?= -m32  $(COPTFLAGS)

all:  reelsaactl

reelsaactl: reelsaactl.o saa7114.o 
	$(CC) $(CFLAGS) $^ -o $@
	strip $@

clean:
	rm -f *~ *.o libsaa7114.so reelsaactl
