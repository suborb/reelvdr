#!/bin/sh
#
# 2006/11/14 by Morone
#
# mp3ng_audiocdripper.sh - v.0.1
#
# A script to start vdrplugin-ripit over SVDRP
#$1 songname
#$2 copydir
#$3 artist
#$4 album
#$5 artistcoverdir
#
#
#
##
### Usage:
### ======
### CHANGE below the path to svdrpsend.pl
### Place an entry in mp3ngcmds.dat like this:
### **********************************************************************************
### Start AudioCD-Ripper?   :  echo "/PATH_TO_SCRIPT/mp3ng_audiocdripper.sh" | at now;
### **********************************************************************************
### thats all and lets hope you get good results ;)
##
#
#
#
#
/VDR/bin/svdrpsend.pl PLUG ripit START
#
#
#######################################EOF
