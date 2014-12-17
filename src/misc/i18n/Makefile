I18Npot = reel.pot

#shell scripts
FILES    = compress_recording.sh \
	delarchivemarks.sh \
	delcuttingmarks.sh \
	delete_preview.sh \
	dvd-rw-format.sh \
		dvdswitch_read.sh dvdswitch_write.sh fskprotect.sh generate_preview.sh \
		handlearchived.sh markasnew.sh mp3_2_media.sh \
		noadcall.sh PTPCameraMount.sh replex2mpeg.sh scast \
		screenshot.sh vdr2mpeg.sh
FILELIST = $(foreach var,$(FILES),../../sbin/$(var))
FILELIST += ../../../packages/usbmount/ROOT/etc/usbmount/mount.d/reelvdr

# perl files
FILESPL    = ripit.pl
FILELISTPL = $(foreach var,$(FILESPL),../../sbin/$(var))


$(I18Npot): $(FILELIST) $(FILELISTPL)
	xgettext -L Shell -cTRANSLATORS --no-wrap -k -ktl -ksmesgtl -ksecho -kgettext --from-code=utf-8\
	    --msgid-bugs-address='<reelbox-devel@mailings.reelbox.org>' $(FILELIST) -o $(I18Npot)
	xgettext -L Perl -cTRANSLATORS --no-wrap -k -kgettext --from-code=utf-8 \
	    --msgid-bugs-address='<reelbox-devel@mailings.reelbox.org>' $(FILELISTPL) -j -o $(I18Npot)
	@for file in ??_??.po ; do \
		echo msgmerge -U --no-wrap --no-location --backup=none -q $@ $$file ;\
		msgmerge -U --no-wrap --backup=none -q $$file $@ ;\
	done

%.po: $(I18Npot)

install: de_DE.po
	sudo msgfmt -c $< -o /usr/share/locale/de_DE/LC_MESSAGES/reel.mo
