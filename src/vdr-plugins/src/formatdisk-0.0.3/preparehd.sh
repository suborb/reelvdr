#!/bin/sh

###### Algo ######
# collect UUIDs belonging to $HDD
# unmount all partitions of device
# destroy partition table
# create new partition
# unmount all partition: because creating partition triggers automount!
# format newly created partition
# remove old UUIDs and partitions from fstab
# trigger UUID update
# new UUID into fstab
# mount partition
# create media subdirectories
# create links to /media/reel # done by calling setup plugin's svdrp command CHMD

# 2012-09-06: new option to download and install system image to a drive
# 2012-03-07: change "nobootwait" option to "nofail" in fstab entries
# 2011-12-15: add "nobootwait" to fstab entries
# 2010-10-04: implement -s for setting the mediadevice
# 2010-06-30: make it modular for use in new setup-hd-wizard


### TODO:
#  format: liefert UUID zurück
#  part:   setzt hdparm / spindown time
#  format: livebuffer einschalten?
. /etc/default/globals

FSTAB=/etc/fstab
OLDUUIDLOG=/tmp/olduuid.$$.log
FORMAT_STATUS_LOG=/tmp/disk_format_status.log
VERSION="V2.2 - 2012-09-06"

syntax () {
cat << EOF

`basename $0` $VERSION
Usage: `basename $0` -d <hd device> [ -t <filesystem> ] [ --all | -b | -f | -p ] [ -c | -g | -h | -o | -y ] [ -l logfile]
Description:
     Prepares unformatted HD for ReelBox

Options:
     -d hd device    single device or partition, i.e. /dev/hdc or /dev/hdd3
     -o              optimize for USB/SSD usage (ext4, no journal)
     -t filesystem   use one of: ext3 | ext4 | jfs | vfat | xfs.  default: jfs
     -l logfile      log to file

Actions:
     --all:          do almost everything (partition, format, mount, basic directories)
     -a:             add partition to device using maximum free space on the disk, becomes automatically formatted
     -b:             write basic directory structure
     -c:             get current mountpoint of the device
     -f:             format device
     -g:             get a free mountpoint under /media
     -h|--help:      this help
     -m|--mount:     mount device, create fstab entry
     -p:             write new partition table to device
     -s:             set this device a mediadevice in sysconfig
     -u|--unmount    unmount, remove from fstab
     -y|--systemdisk create bootable system on this disk

EOF
}



# unmounts a partition from all its mount points
# gives up after 10 tries
unmount_partition ()
{
    PART=$1
    tries=0
    while [ "$(mount | grep $PART)" ];
    do
    	if [ $tries = 0 ] ; then
        	umount  $PART
        else
        	umount -f $PART
        fi
        err=$?
        #[ $? ] && ((tries++))
        tries=$(($tries+1));
        if [ $tries -ge 10 ]; then
            break;
        fi
        busybox usleep 250000
    done
    return $err
}



#umount all partitions of a given device
# eg. unmount_all_partitions /dev/sdc
unmount_all_partitions ()
{
    D=${1##*/} # get 'sdc' from /dev/sdc, remove all characters upto last '/'
    echo "unmount all partitions : $1 ($D)"
    #fdisk -l $1 | grep ^/dev | awk '{print $1}' | xargs -I '{}' unmount_paritition '{}'
    #for p in $( fdisk -l $1 | grep ^/dev | awk '{print $1}' )
    for p in $(cat /proc/partitions | grep "$D[0-9].*" | awk '{print $4}')
    do
        #echo "unmounting $p"
        if ! unmount_partition /dev/$p ; then
        	Syslog "unmounting partition $p failed. Abort."
        	echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
        	$LOG && echo "failure" > $LOGFILE
        	exit 1
        fi
    done
}


remove_from_fstab()
{
    UUIDLIST=$2
    HDD=$1

#    REMOVE_UUID=/tmp/remove_uuid.$$.sh
#cat > ${REMOVE_UUID} << EOF
#eval \$1
#sed 's%\${UUID}%%' -i $FSTAB
#EOF

#cat $UUIDLIST | cut -f2- -d':' | xargs -I '{}' sh $REMOVE_UUID '{}'
    for i in "$(cat $UUIDLIST | cut -f2- -d':')"
    do
        eval $i
        sed /$UUID/d -i $FSTAB
    done

    grep -q ${HDD} $FSTAB && sed "\%${HDD}%d" -i $FSTAB

    #rm from /etc/hdparm.conf
    sed -i /$DEVNAME/,/}/d /etc/hdparm.conf

    #is it the media drive?
    if cat /etc/default/sysconfig | grep MEDIADEVICE= | grep $UUID ; then
	    sed -i s/MEDIADEVICE=.*/MEDIADEVICE=\"\"/ /etc/default/sysconfig
    fi

}


# returns 0 when "$1" directory is empty or non-existant
check_empty_dir ()
{
	num_entries=$(ls -1A "$1" 2>/dev/null | wc -l)
	return $num_entries
}

# add a partition at the end of the disk
AddPartition () {
    #set -x
    #DEVNAME=$(echo ${HDD} | sed s%/dev/%%)

    if [ $# -gt 0 ] ; then
	HDD=$1
	##Is_Device=false
    fi

    if IsDevice ${HDD}; then
	#get next partition id
	for i in 1 2 3 4 X ; do
	    #echo "trying ${DEVNAME}$i"
	    if ! cat /proc/partitions | grep -q ${DEVNAME}$i ; then
		#free partition found
		break
	    fi
	done
	if [ $i = X ] ; then
	    echo "no free partition on this device"
	    return
	else
	    echo "echo found free partition ${HDD}$i"
	    PARTNUM=$i
	fi
    else
	#if partition exists, delete it before
	echo "create partition on ${HDD}"
	PARTNUM=$(echo $HDD | cut -b 9)
	if grep -q ${DEVNAME} /proc/partitions ; then
	    echo "partition exists, we have to delete it first."
	    printf "c\nd\n${PARTNUM}\nw\n" | fdisk ${HDDEV}
	    [ $? -gt 0 ] && echo "ERROR: deleting partition failed"
	fi
    fi

    echo "creating new partition ${HDDEV}${PARTNUM}"
    if [ ${PARTNUM} -lt 4 ] ; then
	printf "c\nn\np\n${PARTNUM}\n\n\nw" | fdisk ${HDDEV}
	[ $? -gt 0 ] && echo "ERROR: partitioning failed"
    else
	printf "c\nn\np\n\n\nw" | fdisk ${HDDEV}
	[ $? -gt 0 ] && echo "ERROR: partitioning failed"
    fi
    #re-read partition table
    ReReadPartitions

    echo "Formating device ${HDD}"
    ##Is_Device=false
    FormatDevice ${HDDEV}${PARTNUM}
}


AddToFstab ()
{
	if [ $# -gt 0 ] ; then
	    HDD=$1
	fi

    # new entry into fstab
	eval `blkid ${HDD} | cut -d' '  -f2- `
	if [ -n "$UUID" ] ; then
		if RbMini ; then
		    echo "UUID=$UUID	$MP	auto	defaults	0	2" >> $FSTAB
		else
		    echo "UUID=$UUID	$MP	auto	defaults,nofail	0	3" >> $FSTAB
		fi
		echo "UUID=$UUID" >> $FORMAT_STATUS_LOG
		$LOG && echo "UUID=$UUID" >> $LOGFILE
		echo "new UUID=$UUID"
	else
		Syslog "identifying disk failed, no UUID. Cancelling."
		echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
		$LOG && echo "failure" > $LOGFILE
		exit 1
	fi
}

CheckDevice () {
    #DEVNAME=$(echo ${HDD} | sed s%/dev/%%)
    dsize=$(grep $DEVNAME /proc/partitions | awk '{print $3}' | head -1)
    if [ $dsize -lt $((3910000)) ] ; then
	echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
	$LOG && echo "failure" > $LOGFILE
	Syslog "device to small for image"
	exit 1
    fi

    if readlink /dev/root | grep -q $DEVNAME ; then
	Syslog "device is the current system device. I am not going to overwrite it."
	echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
	$LOG && echo "failure" > $LOGFILE
	exit 2
    fi
    echo "Device $DEVNAME is ok for writing"
}


CreateBasicDirectories ()
{
	#create media subdirectories
	for i in recordings music pictures video video/dvd opt ; do
		install -d -m2775 ${MP}/$i
	done

	# // write UUID to log
        if $Is_Device ; then
        	HDD=${HDD}1
        	Is_Device=false
        fi

	eval `blkid ${HDD} | cut -d' '  -f2- `
	$LOG && echo "UUID=$UUID" >> $LOGFILE
}

#
# create *one* partition over the whole device
#
CreatePartition ()
{
    reelfpctl -blinkled 4

    # disable usbmount during partitioning
    chmod 000 /usr/share/usbmount/usbmount

    if RbMini ; then

    ## blank bootblock+partition table
	dd if=/dev/zero of=${HDD} bs=512 count=1

    ## create new partition
	printf "n\np\n1\n\n\nw\n" | fdisk ${HDD}

    ##vfat
    #if [ "$FS" = "vfat" ]; then
    #	printf "t\nc\nw\n" | fdisk ${HDD}
    #fi
    else
    ## GPT partition table maybe much larger than 512 bytes.
    ## So, blank the partition table using parted.
	parted ${HDD} --script mklabel gpt

    ## Create a 'Linux/Windows data' partition on $HDD
    ## that spans the entire Harddisk
	printf "n\n\n\n\n700\nw\ny\n" | gdisk ${HDD}
    fi

    # udevtrigger # only on Avg
    # udevadm trigger # only on NetClient
    # on NetClient "udevadm trigger" exists (but not on Avg)
    partprobe # update partition table changes, from parted package available on both Avg and NetClient

    # partprobe triggers a mount!
    # So, umount again
    unmount_all_partitions ${HDD}


    # uuid update takes a while ~10secs!
    if RbMini ; then
	    sleep 10 # /dev/disk/by-uuid takes time to get updated on NetClient
    else
	    sleep 2
    fi
    reelfpctl -clearled 4
    # re-enable usbmount
    chmod 755 /usr/share/usbmount/usbmount

}


ConfigHdparm ()
{
	#HDDEV=$(echo $HDD | cut -b 1-8)
	if ! grep -q $HDDEV /etc/hdparm.conf ; then
		/bin/echo -e "\n$HDDEV {\n\tapm = 0\n\tspindown_time = 24\n\tacoustic_management = 128\n}" >> /etc/hdparm.conf
	fi
	## TODO: reload hdparm on AVG without restart
	[ -x /etc/init.d/hdparm ] && /etc/init.d/hdparm restart
}


DoUpdates () {
    ### TODO //
    echo "Doing updates not yet implemented"
}


FormatDevice ()
{
    if [ $# -gt 0 ] ; then
	HDD=$1
	Is_Device=false
    fi
    reelfpctl -blinkled 4

    # disable usbmount during format
    chmod 000 /usr/share/usbmount/usbmount

    if $Is_Device ; then
	    HDD=${HDD}1
	    Is_Device=false
    fi

    # format
    MKFS=$(which mkfs.$FS)

    if $NOJOURNAL ; then
	OPTIONS="-O ^has_journal"
    fi

    case $FS in
	    ext3|ext4|jfs)
		    LABEL=" -L 'Reel Media Drive'"
		    eval nice -n 10 $MKFS $LABEL $OPTIONS -q ${HDD} > /dev/null
		    ;;
	    xfs)
		    LABEL=" -L 'Reel Media'"
		    eval nice -n 10 $MKFS $LABEL -f -q ${HDD} > /dev/null
		    ;;
	    vfat)
		    LABEL=" -n 'Reel Media Drive'"
		    eval nice -n 10 $MKFS $LABEL ${HDD} > /dev/null
		    ;;
    esac

    reelfpctl -clearled 4

    #needs filesystem check, else the next boot takes too long ~10mins with 250GB hdd!
    #fsck ${HDD}1

    # rm from fstab

    # UUID is refreshed after mkfs,
    # but there seems to be some talk about manual triggering of UUID refresh
    # eg. symlinks here /dev/disk/by-uuid/ are not updated after mkfs: needs udevtrigger
    # refresh UUID
    # from http://www.racmar.com/content/view/26/42/
    #blkid

    GetFreeMountpoint
    udevadm trigger

    AddToFstab
    ConfigHdparm

    # Really mount it directly after formatting?
    #make sure mount point exists
    [ -d $MP ] || mkdir -p "$MP"

    #DEBUG
    mount ${HDD} $MP
    # TODO: mount failed

    # re-enable usbmount
    $(sleep 10 ; chmod 755 /usr/share/usbmount/usbmount) &

}

GetCurrentMountpoint ()
{
	if IsDevice ${HDD} ; then
		HDD=${HDD}1
		Is_Device=false
	fi

	AMP=$(grep ^$HDD /proc/mounts | grep /media/hd | awk '{ print $2}')
	if [ -z "$AMP" ] ; then
		echo "$HDD is Not mounted"
	else
		#only use the first mountpoint
		AMP=$(echo $AMP | awk '{print $1}')
		echo "$HDD mounted to $AMP"
	fi
}

GetFreeMountpoint ()
{
    # find a suitable mountpoint
    MP=/media/hd0

    #find all mountpoints of type MEDIAHD
    MEDIAHD="/media/hd"

    # list directories already in use as mountpoints
    MPs=$(mount | awk '{print $(NF-3)}' | grep ${MEDIAHD})


    # find a unused directory of type ${MEDIAHD}$i (eg. /media/hd3)
    # also the directory should be empty.
    # Note: the directory may not exist, which is fine with us.
    for i in "" $(seq 1 20)
    do
	#echo "$0 : trying ${MEDIAHD}$i"

    # check if ${MEDIAHD}$i is already mounted, then to next iteration
    # else if directory is empty OR doesnot exist the we have found our mountpoint.
	    if ! echo $MPs | grep -q "${MEDIAHD}$i" && check_empty_dir ${MEDIAHD}$i ; then
		    break
	    fi
    done

    MP=${MEDIAHD}$i
    echo "Using mountpoint $MP"
}


IsDevice ()
{
    if [ -n "$1" ]; then
	HDD=$1
    fi
    echo $HDD | grep -q "[0-9]"
    #we have to invert the value of grep
    [ $? -eq 1 ] && true || false
}


ReReadPartitions () {
    partprobe # update partition table changes, from parted package available on both Avg and NetClient

    # partprobe triggers a mount!
    # So, umount again
    unmount_all_partitions ${HDD}

    # uuid update takes a while ~10secs!
    if RbMini ; then
	    sleep 10 # /dev/disk/by-uuid takes time to get updated on NetClient
    else
	    sleep 2
    fi
}


WriteNewImage () {
set -x
    ### TODO: check internet connection
    server=download.reelbox.org/software/ReelBox/ice/
    image=Reelbox-ice-latest.img.gz
    backup_image=Reelbox-ice-20120814.img.gz
    found=false
    for url in "http://$server/$image" "http://$server/$backup_image" ; do
	if wget -q --spider $url ; then
	    found=true
	    echo "found image: $url"
	    break
	fi
    done
    if ! $found ; then
	Syslog "No Image found on server. Stop"
	echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
	$LOG && echo "failure" > $LOGFILE
	exit 3
    fi

    echo "downloading and installing image to ${HDD}"
    wget -o /tmp/wget.log $url -O- | gunzip > ${HDD}
    echo "successfully written image to ${HDD}"
 set +x
    return 0
}


get_progress_wget()
{
        #get the last reported progress percentage from wget's log
        tail  /tmp/wget.log  | grep '%' | awk '/%/{for(i=1;i<=NF;++i)if($i~/%/)max=$i} END{print max}'
}

get_download_status()
{
  WGET_PID=$(pidof wget)    # must be before the get_progress_wget() call
  PROG=$(get_progress_wget)
  
  #echo "PID '$WGET_PID'"
  #echo $PROG

  if [ "$PROG" = "100%" ]; then 
	  echo "success"
  else
        if [ -n "$WGET_PID" ] ; then
                echo "ongoing $PROG"
        else
                # wget not running and the file was not downloaded, then report failure
                echo "failure"
        fi
  fi
}

ReportWriteProgress() 
{
set -x
    while [ 1 ] ; do
        prog=$(get_download_status)
        echo ".... $prog"

        # don't report 'success' just yet
	if [ "$prog" = "success" ]; then 
	    break;
	fi

        # report 'failure' and 'ongoing *'
        
	$LOG && echo "$prog" > $LOGFILE
        if [ "$prog" = "failure" ] ; then
             exit 1 
        fi

        sleep 1
    done
set +x
}


#####################################################################################
#
# Global Variables
#
#####################################################################################

ADDPARTITION="false"
BASICSTRUCTURE="false"
FORMAT="false"
LOG="false"
MEDIADEV="false"
MOUNT="false"
NEWPARTITION="false"
NOJOURNAL="false"
PARTITION="false"
SYSTEM="false"
SYSTEM_STAGE2="false"
UNMOUNT="false"

FS="jfs"
HDD=""
AMP=""   # active mountpoint

#####################################################################################
#
# Main
#
#####################################################################################
#set -x

if [ $# -lt 2 ] ; then
	echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
	$LOG && echo "failure" > $LOGFILE
	syntax
	exit 1
fi


while [ $# -gt 0 ]; do
	case $1 in
		--all)
			# do all evil things
			PARTITION="true"
			FORMAT="true"
			BASICSTRUCTURE="true"
			shift
			;;
		-a|--addpartition)
			ADDPARTITION="true"
			shift
			;;
		-b|--basicstructure)
			BASICSTRUCTURE="true"
			shift
			;;
		-c|--getcurmountpoint)
			GetCurrentMountpoint
			shift
			;;
		-d|--device)
			HDD=$2
			shift 2
			# Work on device or single partition?
			echo $HDD | grep -q [0-9]
			Is_Device=$([ $? -eq 1 ] && echo true || echo false)

			#if $IsDevice ; then
			#	echo "Is a full device"
			#else
			#	echo "Is a partition"
			#fi

			# checking for root device
			#if echo $HDD | grep -q sda ; then
			#	echo "not working on $HDD. Exit."
			#	echo "FORMAT_STATUS=\"complete\"" > $FORMAT_STATUS_LOG
			#	$LOG && echo "failure" > $LOGFILE
			#	exit 3
			#fi
			;;
		-f|--format)
			FORMAT="true"
			shift
			;;
		-g|--getfreemountpoint)
			GetFreeMountpoint
			shift
			;;
		-h|--help)
			syntax
			exit 2
			;;
		-i|-t|--filesystem)
			FS=$2
			shift 2
			;;
		-l|--logfile)
			LOG="true"
			LOGFILE="$2"
			:> $LOGFILE
			shift 2
			;;
		-m|--mount)
			MOUNT=true
			shift
			;;
		-o|--optimize)
		        FS=ext4
			NOJOURNAL="true"
			shift
			;;
		-p|--repartition)
			PARTITION="true"
			shift
			;;
		-s|--mediadevice)
			MEDIADEV="true"
			shift
			;;
		-u|--unmount)
			UNMOUNT=true
			shift
			;;
		-y|--systemdisk)
			SYSTEM="true"
			shift
			;;
		-y2|--system2)
			SYSTEM_STAGE2="true"
			shift
			;;
		*)
			Syslog "Error: unknown option $1"
			syntax
			echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
			$LOG && echo "failure" > $LOGFILE
			exit 2
			;;
	esac
done

#
# here the evil things are going on
#

echo "FORMAT_STATUS=\"ongoing\"" > $FORMAT_STATUS_LOG
$LOG && echo "start" > $LOGFILE
#echo log $log logfile $LOGFILE

if [ "$HDD" = "" ]; then
	Syslog "<device> must be given. Abort."
	echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
	$LOG && echo "failure" > $LOGFILE
	exit 1
fi

if ( $FORMAT || $PARTITION ) && $SYSTEM ; then
	Syslog "You must not combine -y with -f or -p. Abort"
	echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
	$LOG && echo "failure" > $LOGFILE
	exit 1
fi

# HDDEV: the name of the raw device including the /dev path (e.g. /dev/sdc)
if IsDevice ${HDD} ; then
	HDDEV=$HDD
else
	HDDEV=$(echo $HDD | cut -b 1-8)
fi

# DEVNAME: just the plain name of the raw device without path (e.g. sdc)
DEVNAME=$(echo ${HDDEV} | sed s%/dev/%%)

if [ ! -b "$HDDEV" ] ; then
	echo "device $HDD does not exist. Abort."
	echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
	$LOG && echo "failure" > $LOGFILE
	exit 1
fi

#save old UUID and later remove it from fstab
if $Is_Device ; then
	eval `blkid ${HDD}1 | cut -d' '  -f2- `
else
	eval `blkid ${HDD} | cut -d' '  -f2- `
fi
OLDUUID=$UUID
blkid | grep ^$HDD > $OLDUUIDLOG

if $ADDPARTITION ; then
        AddPartition
        echo "done"
fi

if $PARTITION ; then
	if $Is_Device ; then
		# remove old entry from fstab
		remove_from_fstab $HDD $OLDUUIDLOG

		#unmount all partitions of this device
		unmount_all_partitions ${HDD}

		CreatePartition
	else
		echo "Cannot re-partition on a single partition. Abort."
		echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
		$LOG && echo "failure" > $LOGFILE
		exit 1
	fi
fi


if $FORMAT ; then
	if $Is_Device ; then
		unmount_all_partitions $HDD
	else
		unmount_partition $HDD
	fi
	set -x
	# TODO: check for fs given, default?
	remove_from_fstab $HDD $OLDUUIDLOG
	FormatDevice
fi


if $SYSTEM ; then
	CheckDevice
	unmount_all_partitions ${HDD}

	# download and write the image in the background
	WriteNewImage & 
	ReportWriteProgress #wait till the writing is done, and report progress 
	
        ###do partprobe?
	ReReadPartitions

	# turn off journals for USB/SSD
	if $NOJOURNAL ; then
	    tune2fs -O ^has_journal ${HDD}1
	    tune2fs -O ^has_journal ${HDD}2
	fi

	fsck -y ${HDD}1
	fsck -y ${HDD}2

	##CreateMediaPartition
	#AddPartition ${HDD}3
	##TODO: get current mountpoint, otherwise this will not work
	#CreateBasicDirectories

	### TODO: error handling?

	# check filesystem
	## TODO
	#DisableCurrentRoot
fi

if $SYSTEM_STAGE2 ; then
    set -x
	err=0
	#add entry to fstab, but on new system
	mount ${HDDEV}1 /mnt
	err=$(($err+$?))
	mount ${HDDEV}2 /mnt/var
	err=$(($err+$?))
	if [ $err -gt 0 ] ; then
		umount -l /mnt
		Syslog "mounting ${HDD} failed"
		echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
		$LOG && echo "failure" > $LOGFILE
		exit 1
	fi
	FSTAB=/mnt/etc/fstab
	MP="/media/hd"
	AddToFstab ${HDDEV}3

	#enable LiveBuffer
	sed -i s/LiveBuffer.*/"LiveBuffer = 1"/ /mnt/etc/vdr/setup.conf
	# set MEDIADEVICE in sysconfig

	## TODO
	umount /mnt/var
	umount /mnt
	# disable root device by modifying the label
fi

if $MOUNT ; then
	echo "Mounting device."
	GetCurrentMountpoint

	# remove old entry from fstab
	remove_from_fstab $HDD $OLDUUIDLOG

	#set -x
	if ! echo $AMP | grep -q "/media/hd" ; then
		AMP=""
	fi
	if [ -z "$AMP" ] ; then
		#try to mount it
		GetFreeMountpoint
		[ -d $MP ] || mkdir -p "$MP"
		fsck -y $HDD
		mount ${HDD} $MP
		if [ $? != 0 ]; then
			Syslog "Mounting $HDD failed. No such partition? Abort."
			echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
			$LOG && echo "failure" > $LOGFILE
			exit 1
		fi
	else
		MP=$AMP
	fi
	AddToFstab
	ConfigHdparm

fi


if $UNMOUNT ; then
	echo "Unmounting device."
	GetCurrentMountpoint
	remove_from_fstab $HDD $OLDUUIDLOG

	umount -f ${HDD}
	if [ $? != 0 ]; then
		Syslog "Removed from fstab but unmounting $HDD failed. Partition in use? Abort."
		echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
		$LOG && echo "failure" > $LOGFILE
		exit 1
	fi
fi

if $BASICSTRUCTURE ; then
	echo "Writing basic directory structure."
	GetCurrentMountpoint
	#set -x
	if [ -z "$AMP" ] ; then
		#try to mount it
		GetFreeMountpoint
		[ -d $MP ] || mkdir -p "$MP"
		fsck -y $HDD
		mount ${HDD} $MP
		if [ $? != 0 ]; then
			Syslog "Mounting $HDD failed. No such partition? Abort."
			echo "FORMAT_STATUS=\"error\"" > $FORMAT_STATUS_LOG
			$LOG && echo "failure" > $LOGFILE
			exit 1
		fi
	else
		MP=$AMP
	fi
	CreateBasicDirectories

	## change sysconfig's MEDIADEVICE
	## call setup-mediadir (.sh)

	#svdrpsend.sh plug setup CHMD "UUID=$UUID"
fi

if $MEDIADEV ; then
	echo "Setting MEDIADEVICE to UUID=$UUID"
	if grep -q MEDIADEVICE=  /etc/default/sysconfig  ; then
		sed -i s/MEDIADEVICE=.*/MEDIADEVICE=\"UUID=$UUID\"/ /etc/default/sysconfig
	else
		echo "MEDIADEVICE=\"UUID=$UUID\"" >> /etc/default/sysconfig
	fi
	sed -i s/MEDIADEVICE_ONLY_RECORDING=.*/MEDIADEVICE_ONLY_RECORDING=\"no\"/ /etc/default/sysconfig


	# link /home
	GetCurrentMountpoint
	if [ -L /home ] && [ ! -z "$AMP" ] && [ -d $AMP/home ] ; then
		rm -f /home
		echo ln -s $AMP/home /
		ln -s $AMP/home /
	fi
fi

# inform vdr to update its recordings list
#touch /media/reel/recordings/.update

echo "FORMAT_STATUS=\"complete\"" > $FORMAT_STATUS_LOG
$LOG && echo "success" > $LOGFILE
$LOG && echo "UUID=$UUID" >> $LOGFILE
exit 0
