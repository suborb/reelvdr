
CC = gcc
CFLAGS = -g -O3 -Wall -m32
DEFINES =
INCLUDES = -I $(BASE)/include/linux_user \
           -I $(BASE)/include
LIBS = #-L$(BASE)/lib/  -losal -lpal -lplatform_config -lpthread -licepm_user

PROG = ice_pm
OBJS = ice_pm.o

all: $(PROG)

%.o: %.c
	$(CC) $(CFLAGS) -c $(DEFINES) $(INCLUDES) $<

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $(PROG)

clean:
	rm -f *\.o $(PROG) *~
