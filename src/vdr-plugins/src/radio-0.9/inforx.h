/*
 * inforx.h - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef _INFORX__H
#define _INFORX__H

#include "radioaudio.h"
#include "radiotools.h"


extern bool RT_MsgShow, RT_PlusShow;
extern rtp_classes rtp_content;
extern bool DoInfoReq;

#define RadioInfofile "radioinfo.dat"

int info_request(int chantid, int chanpid);


#endif
