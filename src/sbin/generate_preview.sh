#!/bin/sh

. /etc/default/globals

# go to recordings directory
cd "$1"

PREVIEW_IMG=preview_vdr.png

if [ -e $PREVIEW_IMG ]
then
  tl "Preview already exists. Doing nothing."
  return # preview already exists; do nothing
fi


if [ -e 00001.ts ]
then
  FILE=00001.ts
  ### count files ###
  FILECOUNT=`ls -1 *.ts | wc -l`
  ### get middle of ts-files ###
  MIDDLE=`echo scale=0\;"$FILECOUNT / 2 + 1" | bc`
  FILE=`printf '%05d' $MIDDLE`
  FILE=`echo $FILE.ts`
elif [ -e 001.vdr ]
then
  FILE=001.vdr
  ### count files ###
  FILECOUNT=`ls -1 *.vdr | grep -v index | grep -v info | grep -v resume | wc -l`
  ### get middle of vdr-files ###
  MIDDLE=`echo scale=0\;"$FILECOUNT / 2 + 1" | bc`
  FILE=`printf '%03d' $MIDDLE`
  FILE=`echo $FILE.vdr`
else
  msg=`tl "no recording found in %s. Exiting" | sed s/%s/$1/`
  smesg "$msg"
  return # no recording found
fi
if [ $MIDDLE -eq 1 ]
then
  ### if recording has only one File, then get the middle of the selected file ###
  DURATION=`ffmpeg -i $FILE 2>&1 | grep "Duration" | cut -d ' ' -f 4 | sed s/,//`
  HOURS=`echo $DURATION | cut -d ":" -f1`
  MINS=`echo $DURATION | cut -d ":" -f2`
  SECS=`echo $DURATION | cut -d ":" -f1 | cut -d "." -f1`
  DURATION_SEC=`echo "($HOURS * 3600 + $MINS * 60 + $SECS) / 2" | bc`
  MSG="${DURATION_SEC}s in File $MIDDLE"
else
  DURATION_SEC=0
  MSG="begin of File $MIDDLE"
fi

## TODO
## Generate preview intelligently (should be useful)
## usually recordings start 3-5 minute before the actual program
##    so the first frame does not represent the program's preview
## Could jump to the middle of the recording
##    that could mean, looking into each of 000xx.ts files to
##    decide which file has the middle frame
##    Even the middle frame might be of a commercial!!
##    Run NoAd and find a non-commercial "middle" frame

## preview _must_ by 126x128;
# as skin can crash if smaller or show a buggy image if larger
# -intra for intra frame
# -vframes 1 : one video frame (in our case, first intra frame)
# -y to overwrite; ffmpeg, by default, waits for user input when it finds output file

PREVIEW_SIZE=126x128

## for the timebeing just look at the first intra frame for preview
ffmpeg -y -intra -vframes 1 -ss $DURATION_SEC -i "$FILE" -vcodec png -an -f rawvideo -s $PREVIEW_SIZE $PREVIEW_IMG && tl "Preview generated" || tl "Error generating preview"
