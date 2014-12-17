#
# Makefile for ReelBox tools
#
#

TOOLS = avr_flash genindex hdshm reelfpctl replex speedcontrol reelbox-control hdfbshot xine-hde 


-include ../Make.config

all:
	# noad-0.6.0/Makefile
	@for i in $(TOOLS); do \
		$(MAKE) -C $$i ;\
	done

clean:
	@for i in $(TOOLS); do \
                $(MAKE) -C $$i clean ;\
        done


noad-0.6.0/Makefile:
	cd $(@D); \
          aclocal ; \
          ./configure --with-mpeginclude=$(TEMP_DIR)/docimage/libs/mpeg2dec-0.4.1-cvs/include \
            --with-mpeglibdir=$(TEMP_DIR)/docimage/libs/mpeg2dec-0.4.1-cvs/libmpeg2/.libs \
            --with-libmpeg2-static
