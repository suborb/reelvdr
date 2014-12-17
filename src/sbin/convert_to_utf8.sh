#!/bin/bash

DEFAULT_FROM_ENC=iso-8859-15
TO_ENC=utf8

REC_DIR="/media/hd/recordings"
CONFIGS_TO_CONVERT="/etc/vdr/timers.conf \
	/etc/vdr/channels.conf \
	/etc/vdr/plugins/epgsearch/epgsearch.conf \
	/etc/vdr/plugins/epgsearch/timersdone.conf \
	/etc/vdr/plugins/epgsearch/epgsearchdone.data \
	/etc/vdr/plugins/extrecmenu.sort.conf"

FORCE=false
CONVERT_CONFIGS=true
INIT=false

Syntax () {
	cat << EOF
`basename $0` [-c charset] [-d directory] [-f] [-h]
Converts recordings directory (and maybe others) to UTF-8
Options:
  -c charset	character set to convert from (default: $DEFAULT_FROM_ENC)
  		only useful with -f
  -d dir	directory to convert (default: $REC_DIR)
  -f		force, even if vdr is already running at UTF-8
  -h		this help
EOF
}

# echo "$0 called with $*" >> /var/log/`basename $0`.log 2>&1

while [ $# -gt 0 ]; do
	case $1 in
		init)
			# started from /etc/init.d , less output
			INIT=true
			shift
			;;
		-c)
			FROM_ENC="$2"
			[ -z $FROM_ENC ] && exit 2
			shift 2
			;;
		-d)
			REC_DIR="$2"
			[ ! -d "$REC_DIR" ] && exit 2
			CONVERT_CONFIGS=false
			shift 2
			;;
		-f)
			FORCE=true
			shift
			;;
		-h|*)
			Syntax
			exit 1
			;;
	esac
done

if [ -z "$FROM_ENC" ]; then
	OSDLANG=$(grep ^OSDLanguage /etc/vdr/setup.conf | cut -d\  -f3)
	case $OSDLANG in
		0|1|3|4|8|10|19)
			FROM_ENC="iso8859-15"
		;;
		2|9|13|14|17|20)
			FROM_ENC="iso8859-2"
		;;
		5|6|7|12|15)
			FROM_ENC="iso8859-1"
		;;
		11)
			FROM_ENC="iso8859-7"
		;;
		16)
			FROM_ENC="iso8859-5"
		;;
		18)
			FROM_ENC="iso8859-13"
		;;
		*)
			FROM_ENC="$DEFAULT_FROM_ENC"
			if ! $FORCE ; then
				$INIT || echo -e "\nVDR is already running at utf8. Nothing to do."
				exit
			fi
		;;
	esac
fi

#set -x

(
echo -e "\nConverting started @ `date`\n"
echo -e "\nConverting from $FROM_ENC to $TO_ENC\n"

if $CONVERT_CONFIGS ;then
	# convert timers.conf and epgsearch-config-stuff
	echo -e "\nConverting timers.conf and epgsearch-configs"
	for j in $CONFIGS_TO_CONVERT ;
	do
		if file -b "$info" | grep -q "UTF-8" ; then
			echo "already UTF8: $j"
		else
			echo "Converting $j"
			mv $j $j.save
			iconv -f $FROM_ENC -t $TO_ENC $j.save -o $j
		fi
	done
fi

# Convert content of info.vdr to info.vdr.utf8 and move info.vdr.utf8 to info.vdr
echo -e "\nConverting every info.vdr..."
for info in `find "$REC_DIR" -xdev -name info.vdr`; do
	if file -b "$info" | grep -q "UTF-8" ; then
		echo "already UTF8: $info"
	else
		echo Converting $info
		mv "$info" "$info".save
		iconv -f $FROM_ENC -t $TO_ENC "$info".save -o "$info"
	fi
done
echo -e "All info.vdr converted.\n"


# Convert file names
echo -e "\nConverting file names..."
# avoid sym-links to mounts
for i in $(find "$REC_DIR" -xdev -maxdepth 1 -type d);
do
	echo moving "$i"
	convmv --notest -r -f $FROM_ENC -t $TO_ENC --exec "mv -vu #1 #2" "$i"
done

echo -e "Converting file names finished.\n"

) | tee -a /var/log/`basename $0`.log 2>&1
