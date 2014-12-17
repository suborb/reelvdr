#
# Makefile for the GraphLCD driver library
#

include ../../../Make.config
include ../Make.config

VERMAJOR = 1
VERMINOR = 0
VERMICRO = 0

BASENAME = libglcddrivers.so

LIBNAME = $(BASENAME).$(VERMAJOR).$(VERMINOR).$(VERMICRO)

OBJS = common.o config.o driver.o drivers.o port.o framebuffer.o st7565_reel.o reel_usbfp.o image.o
#OBJS = common.o config.o driver.o drivers.o port.o simlcd.o framebuffer.o gu140x32f.o gu256x64-372.o gu256x64-3900.o hd61830.o ks0108.o image.o sed1330.o sed1520.o t6963c.o noritake800.o serdisp.o avrctl.o

HEADERS = config.h driver.h drivers.h port.h framebuffer.h st7565_reel.h reel_usbfp.h image.h
#HEADERS = config.h driver.h drivers.h port.h simlcd.h framebuffer.h gu140x32f.h gu256x64-372.h gu256x64-3900.h hd61830.h ks0108.h image.h sed1330.h sed1520.h t6963c.h noritake800.h serdisp.h avrctl.h


### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Targets:

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) -lusb -ldl -Wl,-soname="$(BASENAME).$(VERMAJOR)" -o $@
	ln -sf $(LIBNAME) $(BASENAME)

install: all
	install -d $(LIBDIR)
	install -m 755 $(LIBNAME) $(LIBDIR)/
	install -d $(INCDIR)/glcddrivers
	install -m 644 $(HEADERS) $(INCDIR)/glcddrivers/
	( cd $(LIBDIR); ln -sf $(LIBNAME) $(BASENAME).$(VERMAJOR); ln -sf $(LIBNAME) $(BASENAME) )

uninstall:
	rm -f $(LIBDIR)/$(BASENAME)
	rm -f $(LIBDIR)/$(BASENAME).$(VERMAJOR)
	rm -f $(LIBDIR)/$(LIBNAME)
	(for i in $(HEADERS); do rm -f $(INCDIR)/glcddrivers/$$i; done)
	rmdir $(INCDIR)/glcddrivers

clean:
	rm -f $(OBJS) $(DEPFILE) $(LIBNAME) $(BASENAME) *~

