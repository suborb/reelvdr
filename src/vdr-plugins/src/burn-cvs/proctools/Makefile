CXX      ?= g++
AR       ?= ar

CXXFLAGS ?= -g -O2 -Woverloaded-virtual -Wall -fPIC -D_LARGEFILE_SOURCE

OBJS = logger.o conlogger.o chain.o environment.o process.o shellprocess.o \
       poller.o property.o shellescape.o

INCLUDES = -I..

### Implicit rules:

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cc) > $@

-include $(DEPFILE)

### Targets:

all: libproctools.a

libproctools.a: $(OBJS)
	$(AR) r $@ $^

clean:
	rm -f *.o core* libproctools.a proctest $(DEPFILE)
	rm -rf temp data
