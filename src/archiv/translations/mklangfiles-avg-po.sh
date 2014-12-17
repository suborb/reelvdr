#!/bin/sh

srcpath=../..
dest="origs-`date --iso-8601`"


[ ! -d $dest ] && mkdir $dest

for i in arghdirector \
	bgprocess \
	bouquets \
	burn \
	channelscan \
	dpkg \
	dvdswitch \
	epgsearch \
	extrecmenu \
	femon \
	formatdisk \
	filebrowser \
	games \
	graphlcd \
	install \
	ipod \
	mediad \
	mediaplayer \
	netcv \
	netcvrotor \
	osdteletext \
	pin \
	premiereepg \
	radio \
	reelblog \
	reelbox \
	reelepg \
	reelchannellist \
	ripit \
	setup \
	shoutcast \
	skinreel3 \
	sleeptimer \
	streamdev \
	timeline \
	vlcclient \
	webbrowser \
	xepg \
	xinemediaplayer \
		; do
	case $i in
			*)
			src=$i/po/*.po
			;;
	esac
	
	[ ! -d $dest/$i ] && mkdir $dest/$i
	cp -a $srcpath/vdr-plugins/src/$src $dest/$i/
done

[ ! -d $dest/vdr ] && mkdir $dest/vdr
cp -a $srcpath/vdr-1.7/po/*.po $dest/vdr/

if [ -f langfiles.zip ]; then mv langfiles.zip langfiles.zip.old ; fi

zip langfiles.zip -r $dest README

