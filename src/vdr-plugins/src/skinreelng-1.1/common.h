/*
 * common.h: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __SKINREEL_COMMON_H
#define __SKINREEL_COMMON_H

#include <string>
#include <stdio.h>

#include <vdr/config.h>

// unset HAVE_FREETYPE if VDR 1.5.3
#ifdef HAVE_FREETYPE
#  if VDRVERSNUM == 10503
#    undef HAVE_FREETYPE
#  endif
#endif

#ifdef DEBUG
//#define debug(x...) { printf("ReelNG: " x); printf("\n"); }
//#define error(x...) { fprintf(stderr, "ReelNG: " x); fprintf(stderr, "\n"); }
#define debug(x...) { isyslog("ReelNG: " x);  }
#define error(x...) { esyslog("ReelNG: " x);  }
#else
#define debug(x...) ;
#define error(x...) esyslog("ReelNG: " x);
#endif

#endif // __SKINREEL_COMMON_H
// vim:et:sw=2:ts=2:
