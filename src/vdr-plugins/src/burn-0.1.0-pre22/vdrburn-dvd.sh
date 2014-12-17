#!/bin/sh

# For DEBUG Output - can be left since logfiles are deleted when job finishes
set -x

# To make the script exit whenever a command fails (MUST NOT BE REMOVED)
set -e

# Variables passed in:
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

PROJECTX_CMD=/usr/bin/projectx_nogui
#PROJECTX_CMD=$JAVA_HOME/bin/java -Djava.awt.headless=true -jar $PROJECTX_HOME/ProjectX.jar

CONFIG_PATH="/etc/vdr/plugins/burn"

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
#		REQUANTCMD=""
#		if [ ! -z $REQUANT_FACTOR ]; then
#			REQUANTCMD="requant $REQUANT_FACTOR |"
#		fi

		IGNORE=""
		if [ ! -z $IGNORE_TRACKS ]; then
			IGNORE="-ignore $IGNORE_TRACKS"
		fi

		CUT=""
		if [ ! -z $USE_CUTTING ]; then
			CUT="-cut"
		fi

		vdrsync.pl -o "$MPEG_TMP_PATH" \
			-v-filter "burn-buffers" \
			-a-filter "burn-buffers" \
			-ac3-filter "burn-buffers" \
			$CUT $IGNORE "$RECORDING_PATH/"
	;;

        demuxrx)
                FILES=""
                VID_ARGS=""
                AUD_ARGS=""
                AC3_ARGS=""
                for i in $(echo $USED_TRACKS | tr "," " ") ; do
                        if [ "$(echo $i | grep -i "0xe")" != "" ] ; then
                                VID_ARGS="-v $i"
                        elif [ "$(echo $i | grep -i "0xc")" != "" ] ; then
                                AUD_ARGS="$AUD_ARGS -a $i"
                        elif [ "$(echo $i | grep -i "0xbd")" != "" ] ; then
                                AC3_ARGS="-c 0x80"
                        elif [ "$(echo $i | grep -i "0x80")" != "" ] ; then
                                AC3_ARGS="-c 0x80"
                        fi
                done

                FILES="${RECORDING_PATH}/[0-9][0-9][0-9].vdr"
		replex $VID_ARGS $AUD_ARGS $AC3_ARGS -t DVD -j -o "$MOVIE_FILE" $FILES
        ;;

	demuxpx)
		test -e "$MPEG_TMP_PATH/convert" && rm "$MPEG_TMP_PATH/convert"
		ln -s "$RECORDING_PATH" "$MPEG_TMP_PATH/convert"

		CUT=""
		if [ ! -z $USE_CUTTING ]; then
			CUT="-cut $MPEG_DATA_PATH/px.cut"
		fi

		$PROJECTX_CMD -ini $CONFIG_PATH/ProjectX.ini \
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
		growisofs -use-the-force-luke=tty $SPEED -dvd-compat -Z "$DVD_DEVICE" \
				  -V "$DISC_ID" -dvd-video "$DVDAUTHOR_PATH"
		#mount.sh eject
	;;

	burndircd)
		dvd+rw-format -blank $DVD_DEVICE && echo "blanking successfull"
		SPEED=""
		if [ $BURN_SPEED -gt 0 ]; then
			SPEED="-speed=$BURN_SPEED"
		fi
		mkisofs -V "$DISC_ID" -dvd-video "$DVDAUTHOR_PATH" \
				| cdrecord "dev=$DVD_DEVICE" driveropts=burnfree fs=10m $SPEED -
		mount.sh eject
	;;
 
	pipeiso)
		mkisofs -V "$DISC_ID" -dvd-video "$DVDAUTHOR_PATH" \
			> "$ISO_FILE"
	;;

	burniso) #FIXME: fails if no DVD in drive
		dvd+rw-format -blank $DVD_DEVICE && echo "blanking successfull"
		SPEED=""
		if [ $BURN_SPEED -gt 0 ]; then
			SPEED="-speed=$BURN_SPEED"
		fi
		growisofs -use-the-force-luke=tty $SPEED -dvd-compat \
				  -Z "$DVD_DEVICE=$ISO_FILE"
		#mount.sh eject
	;;

	burnisocd)
		SPEED=""
		if [ $BURN_SPEED -gt 0 ]; then
			SPEED="speed=$(($BURN_SPEED * 4))"
		fi
		cdrecord "dev=$DVD_DEVICE" driveropts=burnfree $SPEED fs=10m "$ISO_PIPE"
		mount.sh eject
	;;

	*)
		echo "Usage: $0 {demux}"
		exit 1
	;;
esac

exit $?
