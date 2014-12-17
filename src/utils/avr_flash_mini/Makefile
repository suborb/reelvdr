DEFINES  =
INCLUDES = 
LIBS     = 
LD_EXTRA_FLAGS = 

CC     = arm-linux-gnueabi-gcc
CFLAGS = -O1
STRIP  = arm-linux-gnueabi-strip

all: avr_flash

avr_flash: avr_flash.c
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) avr_flash.c -o avr_flash
	$(STRIP) $@

clean:
	$(RM) avr_flash *~
