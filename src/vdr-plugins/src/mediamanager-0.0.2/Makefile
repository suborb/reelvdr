#$(LIBDIR)/$@.$(APIVERSION)

ALLHANDLERS = auto-generated/allhandlers.h
HANDLERSINC = auto-generated/MediaHandlers.include
MEDIAHANDLERINC = auto-generated/mediahandler.inc
I18NINC = auto-generated/i18n.inc

all: check

check:
	@if test "COMPILE_HANDLERS" -nt "$(ALLHANDLERS)" ; then \
		echo "*** Updating handler include files ***"; \
		$(MAKE) -f Makefile.plugin clean; \
		echo -e "#ifndef _ALL_HANDLERS_H\n#define _ALL_HANDLERS_H" > $(ALLHANDLERS); \
		echo -e "/* generated automatically do not edit */\n" >> $(ALLHANDLERS); \
		echo -e "/* generated automatically do not edit */\n" > $(MEDIAHANDLERINC); \
		echo -e "/* generated automatically do not edit */\n" > $(I18NINC); \
		echo -e "# generated automatically do not edit\n" > $(HANDLERSINC); \
		for ch in `cat COMPILE_HANDLERS` ; do \
			for h in `find $$ch -name "*_handler.h"` ; do \
				echo "#include \"$$h\"" >> $(ALLHANDLERS); \
			done; \
			echo "include $$ch/Handler.include" >> $(HANDLERSINC); \
			echo "#include \"$$ch/i18n.include\"" >> $(I18NINC); \
			sed -e "s,CLASSNAME=\(.*\),Add(new \1());,g" "$$ch/HandlerClassName" >> $(MEDIAHANDLERINC); \
		done; \
		echo -e "\n#endif" >> $(ALLHANDLERS); \
		touch -r "COMPILE_HANDLERS" "$(ALLHANDLERS)"; \
	fi
	@$(MAKE) -f Makefile.plugin all

clean :
	@$(MAKE) -f Makefile.plugin clean

dist:
	@$(MAKE) -f Makefile.plugin dist
