#
# Makefile for the GraphLCD tool showpic
#

-include ../../Make.config

PRGNAME = convpic

OBJS = $(PRGNAME).o logo.o glcd.o bmp.o tiff.o tuxbox.o

all: $(PRGNAME)
.PHONY: all

# Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

# The main program:

$(PRGNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -rdynamic $(OBJS) $(LIBDIRS) -lstdc++ -o $(PRGNAME)

install: $(PRGNAME)
	install -d $(BINDIR)
	install -m 755 -o root -g root -s $(PRGNAME) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/$(PRGNAME)

clean:
	@-rm -f $(OBJS) $(DEPFILE) $(PRGNAME) *~

