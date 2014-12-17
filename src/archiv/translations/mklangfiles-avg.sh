#!/bin/sh

srcpath=../..
dest="origs-`date --iso-8601`"


[ ! -d $dest ] && mkdir $dest

#xml file for menu + setup menu
cp -a $srcpath/etc/vdr/plugins/setup/setup-lang.xml $dest/


for i in burn \
	reelbox \
	skinreelng \
	graphlcd \
	remote \
	setup \
	extrecmenu \
	channelscan \
	filebrowser \
	epgsearch \
	epgtimers\
	epgsearchonly \
	xinemediaplayer \
	dvdswitch \
	cdda \
	vcd \
	music \
	femon \
	timeline \
	streamdev-client \
	streamdev-server \
	games \
	arghdirector \
	osdteletext \
	sleeptimer \
	mediad \
	premiereepg \
	loadepg \
	pin \
	vlcclient \
	radio \
	reelepg \
	install \
	mediaplayer \
	mediaplayerdvd \
	netcv \
	netcvinfo \
	netcvdiseqc \
	ripit \
	dpkg \
	dpkgopt \
	webbrowser \
	bgprocess \
	xepg \
	ipod \
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
