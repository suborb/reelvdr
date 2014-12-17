/*
 * service.h: 'ReelNG' skin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __REELEPG_SERVICE_H
#define __REELEPG_SERVICE_H

#include <string>
#include <stdio.h>

struct Reelepg_setquery_v1_0
{
      bool save_query;		 // Store query in setup of reelepg
      const char* query;         // search term
      int mode;                  // search mode (0=phrase, 1=and, 2=or, 3=regular expression)
      int channelNr;             // channel number to search in (0=any)
      bool useTitle;             // search in title
      bool useSubTitle;          // search in subtitle
      bool useDescription;       // search in description
//      cOsdMenu* pBrowseMenu;	 // pointer to menu
};

#endif // __REELEPG_SERVICE_H

