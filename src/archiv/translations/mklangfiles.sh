#!/bin/sh

srcpath=../..
dest="origs-`date --iso-8601`"


[ ! -d $dest ] && mkdir $dest

#xml file for menu + setup menu
cp -a $srcpath/etc-lite/video/plugins/setup/setup-lang.xml $dest/


for i in burn \
		arghdirector \
		cdda \
		channelscan \
		dvdswitch \
		epgsearch \
		extrecmenu \
		femon \
		filebrowser \
		formathd \
		games \
		graphlcd \
		install \
		loadepg \
		mp3 \
		osdpip \
		osdteletext \
		pin \
		radio \
		reelbox \
		reelcam \
		reelepg \
		remote \
		rotor \
		setup \
		sleeptimer \
		streamdev \
		subtitles-0.3.10 \
		timeline \
		vcd \
		vdrcd \
		vlcclient \
		; do
	case $i in
		cdda)
			src=$i/${i}_i18n.c
			;;
		femon)
			src=$i/${i}i18n.c
			;;
		games|setup)
			src=$i/i18n.cpp
			;;
		*)
			src=$i/i18n.c
			;;
	esac
	[ ! -d $dest/$i ] && mkdir $dest/$i
	cp -a $srcpath/vdr-plugins/src/$src $dest/$i/
done

[ ! -d $dest/vdr ] && mkdir $dest/vdr
cp -a $srcpath/vdr-1.4/i18n.c $dest/vdr/
cp -a reel $dest

if [ -f langfiles.zip ]; then mv langfiles.zip langfiles.zip.old ; fi

zip langfiles.zip -r $dest README

