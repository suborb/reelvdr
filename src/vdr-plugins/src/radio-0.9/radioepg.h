/*
 * radioeph.h - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef _RADIOEPG__H
#define _RADIOEPG__H

#include <stdio.h>
#include <time.h>
#include "radioaudio.h"

extern bool RT_MsgShow, RT_PlusShow;
extern rtp_classes rtp_content;


/* Premiere */
#define PREMIERERADIO_TID 17            // Premiere changes to SKY, EPG working ???
// EPG-Info
#define PEPG_ARTIST "Interpret: "
#define PEPG_TITEL "Titel: "
#define PEPG_ALBUM "Album: "
#define PEPG_JAHR "Jahr: "
// Klassik
#define PEPG_WERK "Werk: "
#define PEPG_KOMP "Komponist: "
int epg_premiere(const char *epgtitle, const char *epgdescr, time_t epgstart, time_t epgend);


/* Kabel Deutschlang */
#define KDRADIO_TID 10003
// EPG-Info
#define KDEPG_ARTIST "Artist: "
#define KDEPG_TITEL "Song: "
#define KDEPG_ALBUM "Album: "
#define KDEPG_KOMP "Comp: "
int epg_kdg(const char *epgdescr, time_t epgstart, time_t epgend);


/* Unity Media, Kabel */
#define UMRADIO_TID1 111
#define UMRADIO_TID2 131
// EPG-Info
#define UMEPG_SUFFIX "Music Choice bei Unitymedia"
int epg_unitymedia(const char *epgtitle, const char *epgdescr, time_t epgstart, time_t epgend);


#endif
