PLUGIN    = games

VERSION   = 0.6.2

GAMES     = minesweeper \
            tictactoe   \
            tetris      \
            snake       \
            tron
OBJS      = launcher.o  \
            display.o   \
            screen.o    \
            bitmap.o    \
            i18n.o      \
            text.o

VDROBJS   = plugin.o
SDLOBJS   = main.o
DEFINES   += -DVERSION=\"$(VERSION)\"
CFLAGS    ?= -pipe -O2 -Wall -Woverloaded-virtual
LFLAGS    ?= -pthread
VDRDIR    = ../../..
LIBDIR    = ../../lib
DVBDIR    = ../../../DVB

-include $(VDRDIR)/Make.config

PACKAGE   = vdr-games-$(VERSION)
ARCHIVES  = $(shell for GAME in $(GAMES); do echo $$GAME/$$GAME.a; done)
SDLGAMES  = $(shell test -f $(VDRDIR)/config.h || echo true )
#VDRVER    = $(shell grep 'define APIVERSION ' $(VDRDIR)/config.h | \
#                    awk '{ print $$3 }' | sed -e 's/"//g' )
#VDRNUM    = $(shell grep 'define APIVERSNUM ' $(VDRDIR)/config.h | \
#                    awk '{ print $$3 }' | sed -e 's/"//g' )

ifeq "$(SDLGAMES)" "true"

	TARGET    = sdl-games
	OBJS     += $(SDLOBJS)
	CFLAGS   += `sdl-config --cflags`
	LFLAGS   += `sdl-config --libs`
	DEFINES  += -DSDL -DVERBOSE

else

	TARGET    = libvdr-games.so
	OBJS     += $(VDROBJS)
	CFLAGS   += -fPIC -I$(VDRDIR) -I$(DVBDIR)/include
	LFLAGS   += -shared --whole-archive $(ARCHIVES)
	DEFINES  += -D_GNU_SOURCE -DVDRVERSNUM=$(VDRNUM) -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

    #-include $(VDRDIR)/Make.config
    include $(VDRDIR)/Make.common

endif

LDFLAGS += $(LFLAGS)


### Targets:
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp)   > $@


#.cpp.o:
%.o: %.cpp
	@#$(COMPILE.cpp) $(DEFINES) $(INCLUDES) $(CFLAGS) -c $< $(OUTPUT_OPTION)
	$(CXX) $(DEFINES) $(INCLUDES) $(CXXFLAGS) -c $< $(OUTPUT_OPTION)


$(I18Npot): $(wildcard *.cpp)

$(ARCHIVES):
	@for GAME in $(GAMES); do $(MAKE) -C $$GAME all; done

libvdr-games.so: $(ARCHIVES)

libvdr-games.so-org: $(OBJS) $(ARCHIVES)
	echo ARCHIVES: $(ARCHIVES)
	$(CXX) $(OBJS) $(INCLUDES) $(LFLAGS) $(ARCHIVES) -o $@
	@cp $@ $(LIBDIR)/$@.$(VDRVER)

sdl-games: $(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) $(ARCHIVES) -o $@

install: all
	@cp $(LIBDIR)/libvdr-games.so.$(VDRVER) /mnt/vdr/lib/plugins/

tar: clean
	@echo -n "Creating archive ..."
	@cd .. ; cp -a vdr-games games-$(VERSION)
	@cd .. ; for GAME in $(GAMES); do rm -rf games-$(VERSION)/$$GAME/CVS; done
	@cd .. ; rm -rf games-$(VERSION)/CVS
	@cd .. ; tar cfz $(PACKAGE).tar.gz games-$(VERSION)
	@cd .. ; rm -rf games-$(VERSION)
	@echo " done"
	@echo

clean: clean-extra

clean-extra:
	@echo
	@echo -n "Cleaning source tree "
	@for GAME in $(GAMES); do $(MAKE) -C $$GAME clean; done
	@rm -f *.o *.a $(TARGET)
	@echo " done"
	@echo

