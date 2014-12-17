#!/bin/sh
#
# requires: ...topnm, pnmscale, pnmcomp, ppmntsc, ppmtoy4m, mpeg2enc, pnmflip, identify
#

#Ignore rev for cnxt driver detection (my is rev 30)
JHEAD=/usr/sbin/jhead
if [ "$(lspci -n | grep 14f1:2450 | awk '{ print $1 " " $2 }')" = "00:00.0 0600:" ]; then
    if [ "$3$4$5" = "jpg00" ]; then
        # Don't process image on ncl (would be to slow)
	DIR=`dirname "$2"`
	if [ ! -d "$DIR" ]; then
	    mkdir -p "$DIR"
	fi
	if [ -f $JHEAD ]; then
	    echo "($3)Strip exif $1->$2"
	    cp -f "$1" "$2"
	    $JHEAD -purejpg "$2"
	else
	    logger "($3)Only link $1->$2"
	    ln -s "$1" "$2"
	fi
	exit 0
    fi
fi

CNXT_DEV="00:00.0 0600: 14f1:2450 (rev 10)"
if [ "$(lspci -n|grep 14f1)" = "$CNXT_DEV" ]; then 
  PNMSCALE=pnmscalefixed
else
  PNMSCALE=pnmscale
fi

# video format. pal or ntsc
FORMAT=pal

# thumbnail sizes
THUMB_WIDTH=150
THUMB_HEIGHT=150

# target image width/height (taking into account visible screen area)
if [ "$FORMAT" = "ntsc" ]; then
  TW=704
  TH=480
  ASPECT=1:1
else
  TW=$4
  TH=$5
  ASPECT=$6
fi

case $ASPECT in
	1:1)
		aspect=1
		;;
	4:3)
		aspect=2
		;;
	16:9)
		aspect=3
		;;
	*)
		echo "Invalid aspect"
		exit 1
		;;
esac

bgcolor="-black"
#bgcolor="-white" #use only for testing

TMP=/tmp/image_convert.$$.pnm
TMP2=/tmp/image_convert2.$$.pnm
IMG=$1
MPG=$2
FILE_TYPE=$3

DIR=`dirname "$MPG"`
if [ ! -d "$DIR" ]; then
  mkdir -p "$DIR"
fi
#
# get the file type and set the according converter to PNM
#
#FILE_TYPE=`file -i -L -b "$IMG" 2>/dev/null | cut -f2 -d/`
echo $FILE_TYPE

case "$FILE_TYPE" in
	jpg | jpeg)
		TO_PNM=jpegtopnm
		;;
	tiff)
		TO_PNM=tifftopnm
		;;
	bmp | x-bmp)
		TO_PNM=bmptoppm
		;;
	png | x-png)
		TO_PNM=pngtopnm
		;;
	Netpbm | pnm | x-portable-pixmap)
		TO_PNM=cat
		;;
	gif)
		TO_PNM=giftopnm
		;;
	*)
		echo "filetype '$FILE_TYPE' is not supported"
		exit 1
		;;
esac

ORIENTATION=`exiftags -q -d "$IMG" 2>/dev/null | grep -m 1 Orientation | awk -F';' '{ print $2 }'`

case "$ORIENTATION" in
	" 1")
		echo "Normal orientation, nothing to rotate"	#normal (top - left)
		;;
	" 2")
        echo "Orientation: top-right, rotating..."
		ROTATE="-tb"  		#mirror horizontal (top - right)
		;;
	" 3")
        echo "Orientation: bottom-right, rotating..."
		ROTATE="-r180"		#rotate 180 (bottom - right)
		;;
	" 4")
        echo "Orientation: bottom-left, rotating..."
		ROTATE="-lr"	  	#mirror vertical (bottom - left)
		;;
	" 5")
        echo "Orientation: left-top, rotating..."
		ROTATE="-tb -r90"	#mirror horizontal, rotate 270 cw (left - top)
		;;
	" 6")
        echo "Orientation: right-top, rotating..."
		ROTATE="-r270"		#rotate 90 cw (right - top)
		;;
	" 7")
        echo "Orientation: right-bottom, rotating..."
		ROTATE="-tb -r270"	#mirror horizontal, rotate 90 cw (right - bottom)
		;;
	" 8")
        echo "Orientation: left-bottom, rotating..."
		ROTATE="-r90"		#rotate 270 cw (left - bottom)
		;;
	*)
		echo "No useful information about orientation available"
		;;
esac

#
# extract the image size & compute scale value
#
LANG=C # get the decimal point right

echo "before TO_PNM"
date

$TO_PNM "$IMG" >$TMP2 #2>/dev/null
#get resolution of orig image
S=`pnmfile $TMP2 | awk '{ printf "%d %d ",$4,$6 }'`

#
# Kl:convert to rgb if grayscale image
#
MAGIC=`head -c2 $TMP2`
if [ "$MAGIC" = "P5" ]; then
 pgmtoppm rgb:ff/ff/ff $TMP2 >$TMP
else
 cp $TMP2 $TMP
fi

# TB: create thumbnail
#
# anytopnm "$1" | pnmscale -xysize $THUMB_HEIGHT $THUMB_WIDTH | pnmtopng > "$1".thumb.png

#
# now run the conversion
case "$ORIENTATION" in
	" 2" | " 3" | " 4")
		#calc scale factor
		#S=`echo $S $TW $TH | awk '{ sw=$3/$1; sh=$4/$2; s=(sw<sh)?sw:sh; printf "%.4f\n",(s>1)?1.0:s; }'`
		S=`echo $S $TW $TH | awk '{ sw=$3/$1; sh=$4/$2; s=(sw<sh)?sw:sh; printf "%.4f\n",s; }'`
		if [ "$FORMAT" = "ntsc" ]; then
			pnmflip $ROTATE $TMP | \
			$PNMSCALE $S | \
			pnmpad $bgcolor -width $TW -height $TH | \
			ppmntsc | \
			ppmtoy4m  -v 0 -n 1 -r -F 30000:1001| \
			mpeg2enc -f 7 -T 90 -F 4 -nn -a 2 -v 0 -o "$MPG"
		else
			pnmflip $ROTATE $TMP | \
			$PNMSCALE $S | \
			pnmpad $bgcolor -width $TW -height $TH | \
			ppmtoy4m -S 420mpeg2 -v 0 -n 1 -r -F 25:1 | \
			mpeg2enc --no-constraints -q 1 -V 1024 -b 50000 -f 9 -F 3 -np -a $aspect -v 0 -o "$MPG"   #RC
			#    mpeg2enc --no-constraints -q 1 -f 9 -F 3 -np -a $aspect -v 0 -o "$MPG"   #RC
		fi
		;;
	" 5" | " 6" | " 7" | " 8")
		#calc scale factor
		#S=`echo $S $TH $TW | awk '{ sw=$3/$1; sh=$4/$2; s=(sw<sh)?sw:sh; printf "%.4f\n",(s>1)?1.0:s; }'`
		S=`echo $S $TH $TW | awk '{ sw=$3/$1; sh=$4/$2; s=(sw<sh)?sw:sh; printf "%.4f\n",s; }'`
		if [ "$FORMAT" = "ntsc" ]; then
			pnmflip $ROTATE $TMP | \
			$PNMSCALE $S | \
			pnmpad $bgcolor -width $TW -height $TH | \
			ppmntsc | \
			ppmtoy4m  -v 0 -n 1 -r -F 30000:1001| \
			mpeg2enc -f 7 -T 90 -F 4 -nn -a 2 -v 0 -o "$MPG"
		else
			pnmflip $ROTATE $TMP | \
			$PNMSCALE $S | \
			pnmpad $bgcolor -width $TW -height $TH | \
			ppmtoy4m -S 420mpeg2 -v 0 -n 1 -r -F 25:1 | \
			mpeg2enc --no-constraints -q 1 -V 1024 -b 50000 -f 9 -F 3 -np -a $aspect -v 0 -o "$MPG"   #RC
			#    mpeg2enc --no-constraints -q 1 -f 9 -F 3 -np -a $aspect -v 0 -o "$MPG"   #RC
		fi
		;;
	*)
		#calc scale factor
		#S=`echo $S $TW $TH | awk '{ sw=$3/$1; sh=$4/$2; s=(sw<sh)?sw:sh; printf "%.4f\n",(s>1)?1.0:s; }'`
		S=`echo $S $TW $TH | awk '{ sw=$3/$1; sh=$4/$2; s=(sw<sh)?sw:sh; printf "%.4f\n",s; }'`
		if [ "$FORMAT" = "ntsc" ]; then
			$PNMSCALE $S $TMP | \
			pnmpad $bgcolor -width $TW -height $TH | \
			ppmntsc | \
			ppmtoy4m  -v 0 -n 1 -r -F 30000:1001| \
			mpeg2enc -f 7 -T 90 -F 4 -nn -a 2 -v 0 -o "$MPG"
		else
			$PNMSCALE $S $TMP | \
			pnmpad $bgcolor -width $TW -height $TH | \
			ppmtoy4m -S 420mpeg2 -v 0 -n 1 -r -F 25:1 | \
			mpeg2enc --no-constraints -q 1 -V 1024 -b 50000 -f 9 -F 3 -np -a $aspect -v 0 -o "$MPG"   #RC
			#    mpeg2enc --no-constraints -q 1 -f 9 -F 3 -np -a $aspect -v 0 -o "$MPG"   #RC
		fi
		;;
esac

echo "finished"

#
# cleanup
#
rm $TMP
rm $TMP2
