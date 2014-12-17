#
# Makefile for epgsearch, a Video Disk Recorder plugin
#
# Christian Wieninger cwieninger at gmx.de
#

PLUGIN = epgsearch

MAINMENUSHORTCUT = epgsearchmainmenushortcut
PLUGIN2 = epgsearchonly
PLUGIN3 = conflictcheckonly
PLUGIN4 = quickepgsearch
PLUGIN5 = epgtimers


### The object files (add further files here):

OBJS = afuzzy.o blacklist.o changrp.o confdloader.o conflictcheck.o conflictcheck_thread.o \
	distance.o $(PLUGIN).o epgsearchcats.o epgsearchcfg.o epgsearchext.o epgsearchsetup.o \
	epgsearchsvdrp.o epgsearchtools.o mail.o md5.o \
	menu_announcelist.o menu_blacklistedit.o menu_blacklists.o menu_commands.o \
	menu_conflictcheck.o menu_deftimercheckmethod.o menu_dirselect.o menu_epgtimers.o \
	menu_event.o menu_favorites.o menu_main.o menu_myedittimer.o menu_quicksearch.o \
	menu_recsdone.o menu_search.o menu_searchactions.o menu_searchedit.o \
	menu_searchresults.o menu_searchtemplate.o menu_switchtimers.o menu_templateedit.o \
	menu_timersdone.o menu_whatson.o noannounce.o rcfile.o  recdone.o recstatus.o \
	searchtimer_thread.o services.o switchtimer.o switchtimer_thread.o templatefile.o \
	timer_thread.o timerdone.o timerstatus.o uservars.o varparser.o


### uncomment one of these lines, if you don't want the addon epgsearchonly, conflictcheckonly
#WITHOUT_EPGSEARCHONLY=1
#WITHOUT_CONFLICTCHECKONLY=1
WITHOUT_QUICKSEARCH = 1
WITHOUT_CREATECATS = 1

# uncomment this if you want to use Perl compatible regular expressions (PCRE),
# also have a look at INSTALL for further notes on this
#HAVE_PCREPOSIX=1

# uncomment this if you want to use libtre for unlimited fuzzy searching,
# also have a look at INSTALL for further notes on this
#HAVE_LIBTRE=1

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### some special defines, have to be set before Make.common
ifdef USEMYSQL
  OBJS += epgsearchmysql.o
  ifdef RBMINI
    INCLUDES += -I/usr/arm-linux-gnueabi/include
    LIBS += -Wl,-Bsymbolic-functions -L/usr/arm-linux-gnueabi/lib/mysql -lmysqlclient
  else
    INCLUDES += `mysql_config --cflags`
    LIBS += `mysql_config --libs_r`
  endif
endif


### Common Makefile rules for reelvdr

-include $(VDRDIR)/Make.common


ALL = libvdr-$(PLUGIN).so 

ifndef WITHOUT_CREATECATS
  ALL += createcats
endif
ifndef WITHOUT_EPGSEARCHONLY
  ALL += libvdr-$(PLUGIN2).so
endif
ifndef WITHOUT_CONFLICTCHECKONLY
  ALL += libvdr-$(PLUGIN3).so
endif
ifndef WITHOUT_QUICKSEARCH
  ALL += libvdr-$(PLUGIN4).so
endif
ifndef WITHOUT_EPGTIMERS
  ALL += libvdr-$(PLUGIN5).so
endif

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

DEFINES1 += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'
DEFINES2 += -DPLUGIN_NAME_I18N='"$(PLUGIN2)"'
DEFINES3 += -DPLUGIN_NAME_I18N='"$(PLUGIN3)"'
DEFINES4 += -DPLUGIN_NAME_I18N='"$(PLUGIN4)"'
DEFINES5 += -DPLUGIN_NAME_I18N='"$(PLUGIN5)"'


ifdef HAVE_PCREPOSIX
  LIBS += -L/usr/lib -lpcreposix -lpcre
  DEFINES += -DHAVE_PCREPOSIX
endif

#ifdef HAVE_LIBTRE
#LIBS += -L/usr/lib -ltre
#DEFINES += -DHAVE_LIBTRE
#endif

ifdef CFLC
  DEFINES += -DCFLC
endif

ifdef DEBUG_CONFL
DEFINES += -DDEBUG_CONFL
endif

ifdef MYSQL_DEBUG
DEFINES += -DMYSQL_DEBUG
endif

OBJS2    = mainmenushortcut.o epgsearchonly.o
LIBS2    =

OBJS3    = mainmenushortcut.o conflictcheckonly.o
LIBS3    =

OBJS4    = mainmenushortcut.o quickepgsearch.o
LIBS4    =

OBJS5    = epgtimers.o
LIBS5    =


### Special rules:

mainmenushortcut.o: mainmenushortcut.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(DEFINES1) $(INCLUDES) $<
epgsearchonly.o: epgsearchonly.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(DEFINES2) $(INCLUDES) $<
conflictcheckonly.o: conflictcheckonly.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(DEFINES3) $(INCLUDES) $<
quickepgsearch.o: quickepgsearch.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(DEFINES4) $(INCLUDES) $<
epgtimers.o: epgtimers.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(DEFINES5) $(INCLUDES) $<


### Targets:
all plug:  $(ALL)


# Dependencies:

$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) $(OBJS2:%.o=%.c) $(OBJS3:%.o=%.c) $(OBJS4:%.o=%.c)> $@

-include $(DEPFILE)


libvdr-$(PLUGIN2).so: $(OBJS2)
	$(CXX) $(CXXFLAGS) -shared $(OBJS2) $(LIBS2) -o $@
	cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)

libvdr-$(PLUGIN3).so: $(OBJS3)
	$(CXX) $(CXXFLAGS) -shared $(OBJS3) $(LIBS3) -o $@
	cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)

libvdr-$(PLUGIN4).so: $(OBJS4)
	$(CXX) $(CXXFLAGS) -shared $(OBJS4) $(LIBS4) -o $@
	cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)

libvdr-$(PLUGIN5).so: $(OBJS5)
	$(CXX) $(CXXFLAGS) -shared $(OBJS5) $(LIBS5) -o $@
	cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)


createcats: createcats.o
	$(CXX) $(CXXFLAGS) createcats.o -o $@

epgsearch-dist: generate-i18n clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)/doc-src
	@-rm -rf $(TMPDIR)/$(ARCHIVE)/html
	@-rm -rf $(TMPDIR)/$(ARCHIVE)/docsrc2man.sh
	@-rm -rf $(TMPDIR)/$(ARCHIVE)/docsrc2html.sh
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

distfull: generate-i18n clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo complete distribution package created as $(PACKAGE).tgz

install-doc:
	@mkdir -p $(MANDIR)/man1
	@mkdir -p $(MANDIR)/man4
	@mkdir -p $(MANDIR)/man5
	@mkdir -p $(MANDIR)/de/man1
	@mkdir -p $(MANDIR)/de/man5

	@cp man/en/*1.gz $(MANDIR)/man1/
	@cp man/en/*4.gz $(MANDIR)/man4/
	@cp man/en/*5.gz $(MANDIR)/man5/
	@cp man/de/*1.gz $(MANDIR)/de/man1/
	@cp man/de/*5.gz $(MANDIR)/de/man5/

clean: clean-plug

clean-plug:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(DEPFILE) *.so *.tgz core* *~ createcats createcats.o pod2*.tmp

