/*
 * mediadebug.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_DEBUG_H
#define _MEDIA_DEBUG_H

#include <stdio.h>
#include <vdr/tools.h>
#include "maintainer.h"

 #if defined(MAINTAINER) /* print to console */
   /* always show errors */
   #define DBG_E(a...) { printf(a); printf("\n"); }

   #if defined(DEBUG_I)
     #define DBG_I(a...) DBG_E(a)
   #else
	 #define DBG_I(a...) ;
   #endif

   #if defined(DEBUG_D)
     #define DBG_D(a...) DBG_E(a)
   #else
	 #define DBG_D(a...) ;
   #endif

 #else  /* log to vdr */

   /* always show errors */
   #define DBG_E(a...) esyslog(a);

   #if defined(DEBUG_I)
     #define DBG_I(a...) isyslog(a);
   #else
	 #define DBG_I(a...) ;
   #endif

   #if defined(DEBUG_D)
     #define DBG_D(a...) dsyslog(a);
   #else
	 #define DBG_D(a...) ;
   #endif

 #endif

#endif
