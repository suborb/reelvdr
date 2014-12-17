# Set by batch file
#PATH:=/MinGW/bin/:$(PATH)
CC:=gcc

#Uncomment the following line to enable debugging output
#DEBUG=1
#VERBOSE=1
#API_SOCK=1

DEFS=-DCLIENT -DLIBRARY -DWIN32 -D_REENTRANT -D_GNU_SOURCE

ifdef VERBOSE
DEFS:= $(DEFS) -DDEBUG
DEBUG=1
endif

CFLAGS:= $(DEFS) -O2 -Wall -I../win32/include -I../../common $(CFLAGS)
LDFLAGS:= -s -L../win32/lib $(LDFLAGS)
LDLIBS:= -liphlpapi -lpthreadGC2 -lws2_32 -lxml2 -lz

ifdef DEBUG
LDFLAGS:= $(LDFLAGS) -g
CFLAGS:= $(CFLAGS) -g 
else
#CFLAGS:= $(CFLAGS) -Os
endif

MCLI = mcli
MCLI_OBJS = mld_common.o mld_client.o mld_reporter.o mcast.o recv_tv.o recv_ccpp.o tools.o tca_handler.o tra_handler.o satlists.o interfaces.o inet_pton.o inet_ntop.o inet_aton.o poll.o
MCLI_SOBJS := main.o dummy_client.o api_server.o input.o

all: lib$(MCLI).a $(MCLI).exe

lib$(MCLI).a: $(MCLI_OBJS) 
	$(AR) $(ARFLAGS) $@ $(MCLI_OBJS)
	@copy /b $@ ..\\win32\\lib\\
	@echo "You can find all libraries in directory ..\\win32\lib"
	@echo "If you need a shared library you need MSVC installed."
	@echo "To build the shared library execute the following:"
	@echo "mingw32-make libmcli.dll"

lib$(MCLI).dll: $(MCLI_OBJS) 
	@echo You need MSVC lib.exe for shared libraries!
	$(CC) $(LDFLAGS) -shared -o $@ $(MCLI_OBJS) $(LDLIBS) -Wl,--out-implib,lib$(MCLI).a -Wl,--output-def,lib$(MCLI).def
	@copy /b $@ ..\\win32\\lib\\
	@copy /b lib$(MCLI).def ..\\win32\\lib\\
	@lib.exe /def:lib$(MCLI).def /machine:x86 /out:..\\win32\\lib\\lib$(MCLI).lib
	@echo "You can find all libraries in directory ..\\win32\lib"

$(MCLI).exe: $(MCLI_OBJS) $(MCLI_SOBJS)
	$(CC) $(LDFLAGS) -o $@ $(MCLI_OBJS) $(MCLI_SOBJS) $(LDLIBS) 

$(MCLI)-shared: lib$(MCLI)
	$(CC) -c $(CFLAGS) -ULIBRARY -o main.o main.c
	$(CC) -c $(CFLAGS) -ULIBRARY -o dummy_client.o dummy_client.c
	$(CC) $(LDFLAGS) -o $@ $(MCLI_SOBJS) $(LDLIBS) -lmcli

clean:
	@-del  $(MCLI)*.exe lib$(MCLI).* *.lib *.o *.la *~
	@-del ..\\win32\\lib\\lib$(MCLI).*

%.o: ../%.c
	$(CC) -c $(CFLAGS) -o $@ $<

