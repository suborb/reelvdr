/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifndef __COMMON_DVD_H
#define __COMMON_DVD_H

//#define CTRLDEBUG
//#define DVDDEBUG
//#define PTSDEBUG
//#define AUDIOIDDEBUG
//#define AUDIOPLAYDEBUG
//#define NAVDEBUG
//#define SUBPDEBUG
//#define IFRAMEDEBUG
//#define IFRAMEDEBUG2

#ifdef DVDDEBUG
#define DEBUGDVD(format, args...) printf (format, ## args)
#else
#define DEBUGDVD(format, args...)
#endif
#ifdef PTSDEBUG
#define DEBUGPTS(format, args...) printf (format, ## args)
#else
#define DEBUGPTS(format, args...)
#endif
#ifdef AUDIOIDDEBUG
#define DEBUG_AUDIO_ID(format, args...) printf (format, ## args)
#else
#define DEBUG_AUDIO_ID(format, args...)
#endif
#ifdef CTRLDEBUG
#define DEBUG_CONTROL(format, args...) printf (format, ## args)
#else
#define DEBUG_CONTROL(format, args...)
#endif
#ifdef AUDIOPLAYDEBUG
#define DEBUG_AUDIO_PLAY(format, args...) printf (format, ## args)
#else
#define DEBUG_AUDIO_PLAY(format, args...)
#endif
#ifdef AUDIOPLAYDEBUG2
#define DEBUG_AUDIO_PLAY2(format, args...) printf (format, ## args)
#else
#define DEBUG_AUDIO_PLAY2(format, args...)
#endif
#ifdef NAVDEBUG
#define DEBUG_NAV(format, args...) printf (format, ## args)
#else
#define DEBUG_NAV(format, args...)
#endif
#ifdef NAV0DEBUG
#define DEBUG_NAV0(format, args...) printf (format, ## args); fflush(NULL)
#else
#define DEBUG_NAV0(format, args...)
#endif
#ifdef SUBPDEBUG
#define DEBUG_SUBP_ID(format, args...) printf (format, ## args)
#else
#define DEBUG_SUBP_ID(format, args...)
#endif
#ifdef SUBPDECDEBUG
#define DEBUG_SUBP_DEC(format, args...) printf (format, ## args)
#else
#define DEBUG_SUBP_DEC(format, args...)
#endif

#ifdef IFRAMEDEBUG
#define DEBUG_IFRAME(format, args...) printf (format, ## args)
#else
#define DEBUG_IFRAME(format, args...)
#endif
#ifdef IFRAMEDEBUG2
#define DEBUG_IFRAME2(format, args...) printf (format, ## args)
#else
#define DEBUG_IFRAME2(format, args...)
#endif

#ifdef PTSDEBUG
#define DEBUG_PTS(format, args...) printf (format, ## args)
#else
#define DEBUG_PTS(format, args...)
#endif
#ifdef PTSDEBUG2
#define DEBUG_PTS2(format, args...) printf (format, ## args)
#else
#define DEBUG_PTS2(format, args...)
#endif

// display error message with parameters on OSD
#define EOSD(fmt,parms...)     {  char msg[132]; \
                                  snprintf(msg, sizeof msg, fmt, parms); \
                                  Skins.Message(mtError,msg); \
                                  Skins.Message(mtError,msg); /* repeat once */ }


#endif // __COMMON_DVD_H
