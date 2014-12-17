#!/bin/bash

# For DEBUG Output - can be left since logfiles are deleted when job finishes
set -x

# To make the script exit whenever a command fails (MUST NOT BE REMOVED)
set -e

# TODO
# Variables passed in (not up-to-date):
# $RECORDING_PATH           Path of VDR recording (/video0/%Bla/2004-01-...rec)
# $MPEG_PATH                Path where streams and movie are stored
# $VIDEO_FILE               Full path- and filename of demuxed video stream
# $AUDIO_FILES              Space-separated list of demuxed audio streams
# $MOVIE_FILE               Full path- and filename of muxed movie
# $REQUANT_FACTOR           Factor by that video shall be shrinked
# $IGNORE_TRACKS            Comma-separated list of audio track IDs to be
#                           ignored
# $USED_TRACKS              Comma-separated list of audio track IDs to be used
# $DVDAUTHOR_XML            Full path- and filename of the DVDAuthor XML
# $DVDAUTHOR_PATH           Full path of the written DVD structure
# $TRACK_ON_DVD             Number of track on dvd in which VDR recording is
#                           saved (2 digits)
# $ISO_FILE                 Full path- and filename of target ISO
# $ISO_PIPE                 Fifo where the created ISO should be piped into
# $DVD_DEVICE               Full path- and filename of the DVD burner device
# $CONFIG_PATH              Full path to burn's config directory
# $TEMP_PATH                Full path to burn's temp directory (namely the same
#                           as $MPEG_TMP_PATH)

if [ -z $JAVA_HOME ]; then
	export JAVA_HOME=/opt/j2re1.4.2
fi

if [ -z $PROJECTX_HOME ]; then
	export PROJECTX_HOME=/opt/ProjectX
fi

if [ -x /usr/bin/wodim ]; then
        CDRECORD="/usr/bin/wodim"
else
        CDRECORD="/usr/bin/cdrecord"
fi

echo "`date` $0 called with $*" >> /tmp/dvd-burn.inv
set >> /tmp/dvd-burn.inv

COUNTERFILE="$RECORDINGSDIR/dvd-archive-counter"

# Some versions of growisofs refuse to start when run with sudo, and may
# misdetect this condition if vdr itself is started from a sudo session.
unset SUDO_COMMAND



## Convert a given hex-number to decimal
## hex-number could have leading zeros and/or prefix "0x"
h2d() {
  # remove the lead '0x', if any
  HEX_NUMBER=$(echo $1 | sed s'/0x//' | tr 'a-f' 'A-F') #bc required capital hex digits ?!

  echo "ibase=16; $HEX_NUMBER" | bc
}


## Given a 'list' whose items (hex numbers) are comma separated 
## and a second parameter (hex number) that is to be searched for,
## this function returns 0 to indicate: item found in list otherwise 1
##  Example:
##    param1:"0x0044,0x55,42" param2:"0x44", return 0 (found)
##
in_comma_separated_hex_list(){
  LIST=$1
  ITEM=$2  # to be searched
  for item in ${LIST//,/ } # space required before '}', comma-separated to space-separated
  do
    # compare two hex-numbers given as strings may contain leading zeros and/or 0x
    if [ $(h2d $item) -eq $(h2d $ITEM) ]; then
    return 0 # found
    fi
  done
  return 1
}



## Given a list of pids along with their 'prefix'es that indicates their type '-v|-c|-a'
## This function prefixes the pids that are in USED_TRACKS env. variable 
## with the given prefix.
##
## Example:
##    USED_TRACKS="0x44,0x56,0x005a,0x77"
##    param="0x44 0x33 0x5a" prefix="-a"
##    then the 'return' value should be
##    "-a 0x44 -a 0x5a"
filter_unused_pids()
{
    ARGS="" 
    PIDS="$1" # "0x67 0x66"
    PREFIX="$2" # -v | -c | -a
    echo "filter_unused_pids() PIDS:$PIDS; PREFIX:$PREFIX" >> /tmp/item.log
    for p in $PIDS
    do
      echo "Checking $p in $USED_TRACKS" >> /tmp/item.log
      in_comma_separated_hex_list $USED_TRACKS $p 
      if [ $? == 0 ]; then
	ARGS="$ARGS $PREFIX $p"
      fi
    done
  echo $ARGS
}


## This function gets a recording-dir as param and 
## it scans the first 1MB of 00001.ts or 001.vdr file 
## for audio and video pids. 
## NOTE: do not output to stdout in this function. 
#       The calling function reads this functions output.
get_replex_pids() {

    REC_FILE="$1/00001.ts"
    test -e "$1/001.vdr" &&  REC_FILE="$1/001.vdr"

    SCAN_FILE=$(mktemp)
    # copy 1MB of the given recording file
    dd if="${REC_FILE}" of="$SCAN_FILE" count=2048 bs=512  2>/dev/null

    SCAN_OUTPUT=$(mktemp)
    replex --scan "$SCAN_FILE" 2>/dev/null > "${SCAN_OUTPUT}"

    # collect video, audio and ac3 stream pids from the output log
    VPIDS=$(cat $SCAN_OUTPUT| awk '/vpid/{print " " $3}' | tr '\n' ' ')
    APIDS=$(cat $SCAN_OUTPUT| awk '/apid/{print " " $3}' | tr '\n' ' ')
    CPIDS=$(cat $SCAN_OUTPUT| awk '/ac3pid/{print " " $3}' | tr '\n' ' ')  # ac3 pids
    
    # remove the temp files
    rm -f -- "${SCAN_FILE}" "${SCAN_OUTPUT}" 2>&1 >/dev/null
    
    # add prefixes to pids as replex expects. One of -v|-c|-a 
    V=$(filter_unused_pids "${VPIDS}" "-v")
    A=$(filter_unused_pids "${APIDS}" "-a")
    C=$(filter_unused_pids "${CPIDS}" "-c")

    echo "$V $A $C" 
}


case $1 in
	render)
		FIFO=/var/run/mplex.fifo
		if [ ! -p $FIFO ] ; then
			rm -f $FIFO
			mkfifo $FIFO
		fi
		png2yuv -j "$MENU_BACKGROUND" -f 25 -n 1 -I t -L 1 | \
		mpeg2enc -f 8 -n p -o "$MENU_M2V"
		mplex -f 8 -o $FIFO "$MENU_M2V" "$MENU_SOUNDTRACK" | \
		spumux -v 2 "$MENU_XML" < $FIFO > "$MENU_MPEG"
	;;

	demux)
		IGNORE=""
		if [ ! -z $IGNORE_TRACKS ]; then
			IGNORE="-ignore $IGNORE_TRACKS"
		fi

		CUT=""
		if [ ! -z $USE_CUTTING ]; then
			CUT="-cut"
		fi

		vdrsync -o "$MPEG_TMP_PATH" \
			-v-filter "burn-buffers" \
			-a-filter "burn-buffers" \
			-ac3-filter "burn-buffers" \
			$CUT $IGNORE "$RECORDING_PATH/"
	;;

	demuxrx)
#		FILES=""
#		VID_ARGS=""
#		AUD_ARGS=""
#		AC3_ARGS=""
#		for i in $(echo $USED_TRACKS | tr "," " ") ; do
#			if [ "$(echo $i | grep -i "0xe")" != "" ] ; then
#				VID_ARGS="-v $i"
#			elif [ "$(echo $i | grep -i "0xc")" != "" ] ; then
#				AUD_ARGS="$AUD_ARGS -a $i"
#			elif [ "$(echo $i | grep -i "0xbd")" != "" ] ; then
#				AC3_ARGS="-c 0x80"
#			elif [ "$(echo $i | grep -i "0x80")" != "" ] ; then
#				AC3_ARGS="-c 0x80"
#			fi
#		done
#		# do not pass vid to replex, sometimes vid does not start with "0xe" ?
#		VID_ARGS="" # USED_TRACKS contain audio track id only (!?)

		#FILES="${RECORDING_PATH}/[0-9][0-9][0-9].vdr"
		#test -e  "${RECORDING_PATH}/00001.ts" && FILES="${RECORDING_PATH}/[0-9][0-9][0-9][0-9][0-9].ts"

		REC_FILES="[0-9][0-9][0-9].vdr"
		test -e  "${RECORDING_PATH}/00001.ts" && REC_FILES="[0-9][0-9][0-9][0-9][0-9].ts"

		# get the pids after scanning the first MB of the given recording
		# and discarding the pids that were not in USED_TRACKS
		REPLEX_PIDS=$(get_replex_pids "${RECORDING_PATH}")
		
		# ignore-pts as it causes replex to fail when some recordings 
		# have ac3 stream. eg. ZDF/DasErste
		#replex $REPLEX_PIDS --ignore_PTS -t DVD -j -o "$MOVIE_FILE" $FILES
		
		#RECORDING_PATH may contain spaces! but REC_FILES should be expanded. Therefore quote only RECORDING_PATH.
		replex $REPLEX_PIDS --ignore_PTS -t DVD -j -o "$MOVIE_FILE" "$RECORDING_PATH/"$REC_FILES
		
		#replex $VID_ARGS $AUD_ARGS $AC3_ARGS -t DVD -j -o "$MOVIE_FILE" $FILES


		#replex $VID_ARGS $AUD_ARGS $AC3_ARGS -z -o $MPEG_DATA_PATH/vdrsync $FILES
		#for i in $(seq 0 4) ; do
		#	if [ -e "$MPEG_DATA_PATH/vdrsync${i}.mp2" ]; then
		#		rm -f "$MPEG_DATA_PATH/vdrsync${i}.mpa"
		#		mv "$MPEG_DATA_PATH/vdrsync${i}.mp2" "$MPEG_DATA_PATH/vdrsync${i}.mpa"
		#	fi
		#done
		#if [ -e "$MPEG_DATA_PATH/vdrsync0.ac3" ]; then
		#	rm -f "$MPEG_DATA_PATH/vdrsync.ac3"
		#	mv "$MPEG_DATA_PATH/vdrsync0.ac3" "$MPEG_DATA_PATH/vdrsync.ac3"
		#fi
		#if [ -e "$MPEG_DATA_PATH/vdrsync.mv2" ]; then
		#	rm -f "$VIDEO_FILE"
		#	mv "$MPEG_DATA_PATH/vdrsync.mv2" "$VIDEO_FILE"
		#fi
	;;

	demuxpx)
		test -e "$MPEG_TMP_PATH/convert" && rm "$MPEG_TMP_PATH/convert"
		ln -s "$RECORDING_PATH" "$MPEG_TMP_PATH/convert"

		CUT=""
		if [ ! -z $USE_CUTTING ]; then
			CUT="-cut $MPEG_DATA_PATH/px.cut"
		fi

		$JAVA_HOME/bin/java -Djava.awt.headless=true \
				-jar $PROJECTX_HOME/ProjectX.jar \
				-ini $CONFIG_PATH/ProjectX.ini \
				$CUT -id $USED_TRACKS,0x1f,0x20 \
				-demux -out "$MPEG_DATA_PATH" -name vdrsync \
				$(ls "$MPEG_TMP_PATH/convert/"[0-9][0-9][0-9].vdr)

		for NUM in `seq 1 9`; do
			if [ -e "$MPEG_DATA_PATH/vdrsync[$NUM].mpa" ]; then
				rm -f "$MPEG_DATA_PATH/vdrsync$NUM.mpa"
				mv "$MPEG_DATA_PATH/vdrsync[$NUM].mpa" "$MPEG_DATA_PATH/vdrsync$NUM.mpa"
			fi
		done
	;;

	requant)
		requant $REQUANT_FACTOR 3 $VIDEO_SIZE < "$VIDEO_FILE" > "$REQUANT_FILE"
		rm -f "$VIDEO_FILE"
	;;

	tcrequant)
		tcrequant -f $REQUANT_FACTOR < "$VIDEO_FILE" > "$REQUANT_FILE"
		rm -f "$VIDEO_FILE"
	;;

	mplex)
		MPLEX_OPTS="-S 0"
		(mplex -h 2>&1 | grep -q -- --ignore-seqend-markers) && \
			MPLEX_OPTS="$MPLEX_OPTS -M"

 		### Subtitles
		SON=$(find "$MPEG_DATA_PATH" -name \*.son)
 		SRT=$(find "$MPEG_DATA_PATH" -name \*.srt)
		SUP=$(find "$MPEG_DATA_PATH" -name \*.sup)
		if [ "!" "x$SON" = "x" ]; then
			# spumux.xml generation is based on son2spumux.sh: http://brigitte.dna.fi/~apm/
			echo "<subpictures>" > "$MPEG_DATA_PATH/spumux.xml"
			echo "  <stream>" >> "$MPEG_DATA_PATH/spumux.xml"
			cat "$SON" | tail -n +11 | while read l1
			do
				read l2 || exit 1
				x=`echo $l1 | cut -f2 -d\( | awk '{printf("%d", $1);}'`
				y=`echo $l1 | cut -f2 -d\( | awk '{printf("%d", $2);}'`
				t1=`echo $l2 | awk '{t1=substr($2,1,8); t2=substr($2,10,2); printf("%s.%s", t1, t2);}'`
				t2=`echo $l2 | awk '{t1=substr($3,1,8); t2=substr($3,10,2); printf("%s.%s", t1, t2);}'`
				i=`echo $l2 | awk '{printf("%s", $NF);}'`
				echo "    <spu start=\"$t1\"" >> "$MPEG_DATA_PATH/spumux.xml"
				echo "         end=\"$t2\"" >> "$MPEG_DATA_PATH/spumux.xml"
				echo "         image=\"$MPEG_DATA_PATH/$i\"" >> "$MPEG_DATA_PATH/spumux.xml"
				echo "         xoffset=\"$x\" yoffset=\"$y\"" >> "$MPEG_DATA_PATH/spumux.xml"
				echo "         transparent=\"000060\" />" >> "$MPEG_DATA_PATH/spumux.xml"
			done
			echo "  </stream>" >> "$MPEG_DATA_PATH/spumux.xml"
			echo "</subpictures>" >> "$MPEG_DATA_PATH/spumux.xml"
			# spumux.xml done
 			SPU=$MPEG_DATA_PATH
 		elif [ "!" "x$SRT" = "x" ]; then
 			echo "<subpictures>" > "$MPEG_DATA_PATH/spumux.xml"
 			echo "  <stream>" >> "$MPEG_DATA_PATH/spumux.xml"
 			echo "         <textsub filename=\"$SRT\" characterset=\"ISO8859-1\"" >> "$MPEG_DATA_PATH/spumux.xml"
 			echo "         fontsize=\"28.0\" font=\"arial.ttf\" horizontal-alignment=\"center\"" >> "$MPEG_DATA_PATH/spumux.xml"
 			echo "         vertical-alignment=\"bottom\" left-margin=\"60\" right-margin=\"60\"" >> "$MPEG_DATA_PATH/spumux.xml"
 			echo "         top-margin=\"20\" bottom-margin=\"30\" subtitle-fps=\"25\"" >> "$MPEG_DATA_PATH/spumux.xml"
 			echo "         movie-fps=\"25\" movie-width=\"720\" movie-height=\"574\"" >> "$MPEG_DATA_PATH/spumux.xml"
 			echo "      />" >> "$MPEG_DATA_PATH/spumux.xml"
 			echo "  </stream>" >> "$MPEG_DATA_PATH/spumux.xml"
 			echo "</subpictures>" >> "$MPEG_DATA_PATH/spumux.xml"
 			# spumux.xml done
 			SPU=$MPEG_DATA_PATH
		elif [ "!" "x$SUP" = "x" ]; then
			# from http://www.guru-group.fi/~too/sw/m2vmp2cut/
			pxsup2dast "$SUP"*
			SPU=$(find "$MPEG_DATA_PATH" -name \*.d)
		fi

		if [ ! "x$SPU" = "x" -a -f "$SPU/spumux.xml" ]; then
			mkfifo "$MPEG_TMP_PATH/subtmp.mpg"
			mplex -f 8 $MPLEX_OPTS -o "$MPEG_TMP_PATH/subtmp.mpg" "$VIDEO_FILE" $AUDIO_FILES &
			spumux -v 2 "$SPU/spumux.xml" < "$MPEG_TMP_PATH/subtmp.mpg" > "$MOVIE_FILE"
		else
			mplex -f 8 $MPLEX_OPTS -o "$MOVIE_FILE" "$VIDEO_FILE" $AUDIO_FILES
		fi
		### End Subtitles

		rm -f "$VIDEO_FILE" $AUDIO_FILES
	;;

	author)
		dvdauthor -x "$DVDAUTHOR_XML"
	;;

	dmharchive)
		echo "Creating $TEMP_PATH/INDEX_${TRACK_ON_DVD}"
		mkdir -p "$TEMP_PATH/INDEX_${TRACK_ON_DVD}"
		cd "$DVDAUTHOR_PATH/VIDEO_TS/"
    		for i in $(seq 1 99) ; do
			if [ -f "VTS_${TRACK_ON_DVD}_$i.VOB" ] ; then
				vdr_file=`printf "%03d.vdr" $i`
				echo "Linking $DVDAUTHOR_PATH/VIDEO_TS/VTS_${TRACK_ON_DVD}_$i.VOB -> $TEMP_PATH/INDEX_${TRACK_ON_DVD}/$vdr_file"
				ln -s "$DVDAUTHOR_PATH/VIDEO_TS/VTS_${TRACK_ON_DVD}_$i.VOB" "$TEMP_PATH/INDEX_${TRACK_ON_DVD}/$vdr_file"
			else
				break
			fi
		done
	        cd "$TEMP_PATH/INDEX_${TRACK_ON_DVD}"
		genindex
		if [ "$?" != "0" ] ; then
			exit 1
		fi
		echo "Copying $TEMP_PATH/INDEX_${TRACK_ON_DVD}/index.vdr -> $RECORDING_PATH/index_archive.vdr"
		cp "$TEMP_PATH/INDEX_${TRACK_ON_DVD}/index.vdr" "$RECORDING_PATH/index_archive.vdr"
		echo "Moving $TEMP_PATH/INDEX_${TRACK_ON_DVD}/index.vdr -> $DVDAUTHOR_PATH/VIDEO_TS/index_${TRACK_ON_DVD}.vdr"
		mv "$TEMP_PATH/INDEX_${TRACK_ON_DVD}/index.vdr" "$DVDAUTHOR_PATH/VIDEO_TS/index_${TRACK_ON_DVD}.vdr"
		if [ -f "$RECORDING_PATH/info.vdr" ]; then
			echo "Copying $RECORDING_PATH/info.vdr -> $DVDAUTHOR_PATH/VIDEO_TS/info_${TRACK_ON_DVD}.vdr"
			cp "$RECORDING_PATH/info.vdr" "$DVDAUTHOR_PATH/VIDEO_TS/info_${TRACK_ON_DVD}.vdr"
		fi
		echo "Deleting $TEMP_PATH/INDEX_${TRACK_ON_DVD}"
		rm -rf "$TEMP_PATH/INDEX_${TRACK_ON_DVD}"

		echo "Creating $RECORDING_PATH/dvd.vdr"
		if [ ! -f $COUNTERFILE ]; then
			echo "0001" > $COUNTERFILE
		fi

		cp "$COUNTERFILE" "$RECORDING_PATH/dvd.vdr"
		printf "%04d\n" $(echo ${TRACK_ON_DVD} | sed 's/^0*//') >> "$RECORDING_PATH/dvd.vdr"
	;;

	archivemark)
		cp "$COUNTERFILE" "$DVDAUTHOR_PATH/dvd.vdr"
		let $((DVD_ID=`cat $COUNTERFILE | sed "s/^0*//"` + 1))
		printf "%04d\n" $DVD_ID > "$COUNTERFILE"
		cp "$COUNTERFILE" "$CONFIG_PATH/counters/standard"
	;;

	mkiso)
		mkisofs -V "$DISC_ID" -dvd-video "$DVDAUTHOR_PATH" > "$ISO_FILE"
	;;

	burndir)
		dvd+rw-format -blank $DVD_DEVICE && echo "blanking successfull"
		SPEED=""
		if [ $BURN_SPEED -gt 0 ]; then
			SPEED="-speed=$BURN_SPEED"
		fi
		growisofs -use-the-force-luke=tty -use-the-force-luke=notray $SPEED -dvd-compat -Z "$DVD_DEVICE" \
				  -V "$DISC_ID" -dvd-video "$DVDAUTHOR_PATH"
		#mount.sh eject
	;;

	burndircd)
		SPEED=""
		if [ $BURN_SPEED -gt 0 ]; then
			SPEED="speed=$(($BURN_SPEED * 4))"
		fi
		mkisofs -V "$DISC_ID" -dvd-video "$DVDAUTHOR_PATH" \
				| $CDRECORD "dev=$DVD_DEVICE" driveropts=burnfree fs=10m $SPEED -
		#mount.sh eject
	;;

	pipeiso)
		mkisofs -V "$DISC_ID" -dvd-video "$DVDAUTHOR_PATH" \
			> "$ISO_FILE"
	;;

	burniso) #FIXME: fails if no DVD in drive
		dvd+rw-format -blank $DVD_DEVICE && echo "blanking successfull"
		sleep 15
		while [ "$(pidof mkisofs)" != "" ] ; do
			echo "Waiting for ISO"
			sleep 5
		done
		SPEED=""
		if [ $BURN_SPEED -gt 0 ]; then
			SPEED="-speed=$BURN_SPEED"
		fi
		growisofs -use-the-force-luke=tty -use-the-force-luke=notray $SPEED -dvd-compat \
				  -Z "$DVD_DEVICE=$ISO_FILE"
		#mount.sh eject
	;;

	burnisocd)
		SPEED=""
		if [ $BURN_SPEED -gt 0 ]; then
			SPEED="speed=$(($BURN_SPEED * 4))"
		fi
		$CDRECORD "dev=$DVD_DEVICE" driveropts=burnfree $SPEED fs=10m "$ISO_PIPE"
		#mount.sh eject
	;;

	*)
		echo "Usage: $0 {demux}"
		exit 1
	;;
esac

exit $?
