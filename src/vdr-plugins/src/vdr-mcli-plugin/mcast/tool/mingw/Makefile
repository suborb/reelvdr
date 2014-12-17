NETCVDIAG = netcvdiag.exe
NETCVDIAG_OBJS = netcvdiag.o tools.o

NETCVLOGVIEW = netcvlogview.exe
NETCVLOGVIEW_OBJS = netcvlogview.o mcast.o

CC:=gcc
CFLAGS:= -O2 -DCLIENT -DSTATICLIB -Wall -I../../common/win32/include -I../../common -I../../client
LDFLAGS:= -s -L../../common/win32/lib
LDLIBS:=-lmingwex -lpthreadGC2 -lmcli -lxml2 -lz -liphlpapi -lws2_32

all: $(NETCVDIAG) $(NETCVLOGVIEW)
	@echo Note: netcvupdate.exe can't be built without POSIX environment.

$(NETCVDIAG): $(NETCVDIAG_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(NETCVDIAG_OBJS) $(LDLIBS)

$(NETCVLOGVIEW): $(NETCVLOGVIEW_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(NETCVLOGVIEW_OBJS) $(LDLIBS)

%.o: ../%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	@-del  $(NETCVDIAG) $(NETCVLOGVIEW) *.o *.la *~

