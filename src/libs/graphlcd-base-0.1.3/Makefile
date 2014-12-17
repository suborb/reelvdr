#
# Makefile for the GraphLCD driver library, graphics library and tools
#

PROJECT = graphlcd-base
VERSION = 0.1.3
ARCHIVE = $(PROJECT)-$(VERSION)
PACKAGE = $(ARCHIVE)
TMPDIR = /tmp

OBJS = common.o config.o driver.o drivers.o port.o framebuffer.o st7565_reel.o image.o

### Targets:

all:
	@$(MAKE) -C glcdgraphics all
	@$(MAKE) -C glcddrivers all
ifndef RBMINI
	@$(MAKE) -C tools all
endif

install:
	@$(MAKE) -C glcdgraphics install
	@$(MAKE) -C glcddrivers install
	@$(MAKE) -C tools install

uninstall:
	@$(MAKE) -C glcdgraphics uninstall
	@$(MAKE) -C glcddrivers uninstall
	@$(MAKE) -C tools uninstall
  
clean:
	@$(MAKE) -C glcdgraphics clean
	@$(MAKE) -C glcddrivers clean
	@$(MAKE) -C tools clean

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

